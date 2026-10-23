#include "mem.h"
#include "../io/io.h"
#include "../log/log.h"
#include "../vm/vm.h"

static size_t choose_lru_global(Hv *hv) {
    size_t victim = SIZE_MAX;
    if (hv->frame_count == 0U) return SIZE_MAX;
    for (size_t i = 0U; i < hv->frame_count; ++i) {
        if (!hv->frames[i].used || hv->frames[i].pending) continue;
        if (victim == SIZE_MAX || hv->frames[i].last_access < hv->frames[victim].last_access) {
            victim = i;
        }
    }
    return victim;
}

static const MemPolicy LRU_GLOBAL_POLICY = {"LRU_GLOBAL", choose_lru_global};

const MemPolicy *mem_policy_create(const char *algorithm, const char *scope) {
    if (algorithm != NULL && scope != NULL && strcmp(algorithm, "LRU") == 0 && strcmp(scope, "GLOBAL") == 0) {
        return &LRU_GLOBAL_POLICY;
    }
    return NULL;
}

bool mem_init(Hv *hv) {
    if (hv == NULL || hv->cfg.page_words == 0U) return false;
    hv->mem_policy_impl = mem_policy_create(hv->cfg.replacement, hv->cfg.repl_scope);
    if (hv->mem_policy_impl == NULL) {
        (void)fprintf(stderr, "Politica de memoria nao suportada para este escopo\n");
        return false;
    }
    hv->frame_count = hv->cfg.phys_words / hv->cfg.page_words;
    hv->physical_memory = (int32_t *)calloc(hv->cfg.phys_words, sizeof(*hv->physical_memory));
    hv->frames = (Frame *)calloc(hv->frame_count, sizeof(*hv->frames));
    if (hv->physical_memory == NULL || hv->frames == NULL) {
        (void)fprintf(stderr, "Memoria fisica insuficiente para inicializar o MiniHV\n");
        return false;
    }
    for (size_t i = 0; i < hv->frame_count; ++i) hv->frames[i].owner_vm = -1;
    return true;
}

void mem_destroy(Hv *hv) {
    if (hv == NULL) return;
    free(hv->physical_memory); hv->physical_memory = NULL;
    free(hv->frames); hv->frames = NULL;
    hv->frame_count = 0U;
    hv->mem_policy_impl = NULL;
}

void mem_release_vm(Hv *hv, Vm *vm) {
    if (hv == NULL || vm == NULL || hv->frames == NULL) return;
    for (size_t i = 0; i < hv->frame_count; ++i) {
        Frame *frame = &hv->frames[i];
        char detail[96];
        if (!frame->used || frame->owner_vm != vm->id) continue;
        if (vm->pages != NULL && frame->page < vm->page_count) {
            vm->pages[frame->page].present = false;
            vm->pages[frame->page].pending = false;
            vm->pages[frame->page].frame = -1;
            vm->pages[frame->page].referenced = false;
            vm->pages[frame->page].dirty = false;
        }
        (void)snprintf(detail, sizeof(detail), "frame=%lu page=%lu", (unsigned long)i,
                       (unsigned long)frame->page);
        log_event(hv, "INFO", "mem", vm->name, "MEM_FREE", detail);
        memset(&hv->physical_memory[i * hv->cfg.page_words], 0,
               hv->cfg.page_words * sizeof(*hv->physical_memory));
        frame->used = false;
        frame->pending = false;
        frame->owner_vm = -1;
        frame->page = 0U;
        frame->load_tick = 0U;
        frame->last_access = 0U;
    }
}

size_t mem_used_frames(const Hv *hv) {
    size_t used = 0U;
    if (hv == NULL || hv->frames == NULL) return 0U;
    for (size_t i = 0; i < hv->frame_count; ++i) if (hv->frames[i].used) used++;
    return used;
}

static size_t find_frame(Hv *hv) {
    for (size_t i = 0; i < hv->frame_count; ++i) if (!hv->frames[i].used) return i;
    if (hv->mem_policy_impl == NULL) return SIZE_MAX;
    return hv->mem_policy_impl->choose_victim(hv);
}

static bool reserve_page(Hv *hv, Vm *vm, size_t page) {
    size_t frame_index = find_frame(hv);
    Frame *frame;
    PageEntry *pte;
    char detail[160];
    if (frame_index == SIZE_MAX) return false;
    frame = &hv->frames[frame_index];
    if (frame->used) {
        Vm *old_vm = &hv->vms[frame->owner_vm];
        PageEntry *old_pte = &old_vm->pages[frame->page];
        size_t old_base = frame_index * hv->cfg.page_words;
        for (size_t offset = 0; offset < hv->cfg.page_words &&
             frame->page * hv->cfg.page_words + offset < old_vm->mem_words; ++offset) {
            old_vm->backing[frame->page * hv->cfg.page_words + offset] =
                hv->physical_memory[old_base + offset];
        }
        if (old_pte->dirty) hv->dirty_writebacks++;
        hv->replacements++;
        (void)snprintf(detail, sizeof(detail), "page=%lu frame=%lu dirty=%s",
                       (unsigned long)frame->page, (unsigned long)frame_index,
                       old_pte->dirty ? "true" : "false");
        log_event(hv, "INFO", "mem", old_vm->name, "PAGE_EVICT", detail);
        old_pte->present = false;
        old_pte->pending = false;
        old_pte->frame = -1;
        old_pte->referenced = false;
        old_pte->dirty = false;
    }
    frame->used = true;
    frame->pending = true;
    frame->owner_vm = vm->id;
    frame->page = page;
    frame->load_tick = hv->tick;
    frame->last_access = hv->tick;
    pte = &vm->pages[page];
    pte->present = false;
    pte->pending = true;
    pte->frame = (int)frame_index;
    pte->referenced = false;
    pte->dirty = false;
    pte->load_tick = hv->tick;
    pte->last_access = hv->tick;
    pte->accesses = 0U;
    memset(&hv->physical_memory[frame_index * hv->cfg.page_words], 0,
           hv->cfg.page_words * sizeof(*hv->physical_memory));
    return true;
}

/* RF-MEM-04: the page becomes present only when the SWAP interrupt arrives. */
bool mem_complete_page_fault(Hv *hv, Vm *vm, size_t page) {
    PageEntry *pte;
    Frame *frame;
    size_t base;
    char detail[128];
    if (hv == NULL || vm == NULL || page >= vm->page_count || vm->pages == NULL) return false;
    pte = &vm->pages[page];
    if (!pte->pending || pte->frame < 0 || (size_t)pte->frame >= hv->frame_count) return false;
    frame = &hv->frames[(size_t)pte->frame];
    if (!frame->used || !frame->pending || frame->owner_vm != vm->id || frame->page != page) return false;
    base = (size_t)pte->frame * hv->cfg.page_words;
    for (size_t offset = 0U; offset < hv->cfg.page_words; ++offset) {
        size_t virtual_address = page * hv->cfg.page_words + offset;
        hv->physical_memory[base + offset] = virtual_address < vm->mem_words ? vm->backing[virtual_address] : 0;
    }
    frame->pending = false;
    frame->load_tick = hv->tick;
    frame->last_access = hv->tick;
    pte->present = true;
    pte->pending = false;
    pte->referenced = true;
    pte->dirty = false;
    pte->load_tick = hv->tick;
    pte->last_access = hv->tick;
    pte->accesses = 0U;
    (void)snprintf(detail, sizeof(detail), "page=%lu frame=%d", (unsigned long)page, pte->frame);
    log_event(hv, "INFO", "mem", vm->name, "PAGE_IN", detail);
    return true;
}

static MemResult translate(Hv *hv, Vm *vm, int64_t address, int32_t *value,
                           bool write, bool fetch_only) {
    size_t page;
    size_t offset;
    PageEntry *pte;
    Frame *frame;
    char detail[128];
    if (address < 0 || (uint64_t)address >= (uint64_t)vm->mem_words) {
        vm_exit(hv, vm, "PROTECTION_FAULT");
        return MEM_ERROR;
    }
    page = (size_t)address / hv->cfg.page_words;
    offset = (size_t)address % hv->cfg.page_words;
    pte = &vm->pages[page];
    if (!pte->present) {
        if (pte->pending) return MEM_PENDING;
        vm->page_faults++;
        hv->page_faults++;
        if (!reserve_page(hv, vm, page)) {
            vm_exit(hv, vm, "KILLED");
            return MEM_ERROR;
        }
        pte = &vm->pages[page];
        (void)snprintf(detail, sizeof(detail), "page=%lu frame=%d penalty=%d",
                       (unsigned long)page, pte->frame, hv->cfg.pf_penalty);
        log_event(hv, "INFO", "mem", vm->name, "PAGE_FAULT", detail);
        if (!vm_set_state(hv, vm, VM_BLOCKED) ||
            !io_submit(hv, DEV_SWAP, vm->id, 0, 0U, 0, page)) {
            if (vm->state != VM_TERMINATED) vm_exit(hv, vm, "KILLED");
            return MEM_ERROR;
        }
        return MEM_PENDING;
    }
    frame = &hv->frames[(size_t)pte->frame];
    pte->referenced = true;
    pte->accesses++;
    pte->last_access = hv->tick;
    frame->last_access = hv->tick;
    if (!fetch_only) {
        size_t physical_address = (size_t)pte->frame * hv->cfg.page_words + offset;
        if (write) {
            hv->physical_memory[physical_address] = value == NULL ? 0 : *value;
            pte->dirty = true;
        } else if (value != NULL) {
            *value = hv->physical_memory[physical_address];
        }
    }
    return MEM_OK;
}

MemResult mem_fetch_instruction(Hv *hv, Vm *vm, int64_t address) {
    return translate(hv, vm, address, NULL, false, true);
}

MemResult mem_read_word(Hv *hv, Vm *vm, int64_t address, int32_t *value) {
    return translate(hv, vm, address, value, false, false);
}

MemResult mem_write_word(Hv *hv, Vm *vm, int64_t address, int32_t value) {
    return translate(hv, vm, address, &value, true, false);
}

void mem_print_status(const Hv *hv) {
    if (hv == NULL) return;
    (void)printf("Memoria: modelo=PAG, substituicao=LRU GLOBAL, phys_words=%lu, frames_used=%lu/%lu, page_faults=%llu, substituicoes=%llu, writebacks_sujos=%llu\n",
        (unsigned long)hv->cfg.phys_words, (unsigned long)mem_used_frames(hv),
        (unsigned long)hv->frame_count, (unsigned long long)hv->page_faults,
        (unsigned long long)hv->replacements, (unsigned long long)hv->dirty_writebacks);
}
