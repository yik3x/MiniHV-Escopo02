#include "vm.h"
#include "../cpu/loader.h"
#include "../log/log.h"

static const bool valid_transition[VM_STATE_COUNT][VM_STATE_COUNT] = {
    {false, true,  false, false, false, true },
    {false, false, true,  false, true,  true },
    {false, false, false, true,  true,  true },
    {false, true,  false, false, false, true },
    {false, true,  false, false, false, true },
    {false, false, false, false, false, false}
};

bool vm_set_state(Hv *hv, Vm *vm, VmState next) {
    char detail[128];
    VmState old;
    if (hv == NULL || vm == NULL || next < 0 || next >= VM_STATE_COUNT) return false;
    old = vm->state;
    if (!valid_transition[old][next]) {
        (void)snprintf(detail, sizeof(detail), "transicao_invalida=%s->%s", vm_state_name(old), vm_state_name(next));
        log_event(hv, "WARN", "vm", vm->name, "INVALID_STATE_TRANSITION", detail);
        return false;
    }
    vm->state = next;
    (void)snprintf(detail, sizeof(detail), "from=%s to=%s", vm_state_name(old), vm_state_name(next));
    log_event(hv, "INFO", "vm", vm->name, "STATE_CHANGE", detail);
    return true;
}

void vm_exit(Hv *hv, Vm *vm, const char *reason) {
    if (hv == NULL || vm == NULL || vm->state == VM_TERMINATED) return;
    (void)snprintf(vm->exit_reason, sizeof(vm->exit_reason), "%s", reason == NULL ? "KILLED" : reason);
    if (!vm_set_state(hv, vm, VM_TERMINATED)) return;
    vm->finish = hv->tick;
    log_event(hv, "INFO", "vm", vm->name, "VM_EXIT", vm->exit_reason);
}

bool vm_init_all(Hv *hv) {
    if (hv == NULL) return false;
    hv->vm_count = hv->cfg.vm_count;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        Vm *vm = &hv->vms[i];
        char error[512];
        size_t bytes;
        memset(vm, 0, sizeof(*vm));
        vm->id = (int)i;
        (void)snprintf(vm->name, sizeof(vm->name), "%s", hv->cfg.vms[i].name);
        (void)snprintf(vm->program_path, sizeof(vm->program_path), "%s", hv->cfg.vms[i].program);
        vm->state = VM_NEW;
        vm->arrival = hv->cfg.vms[i].arrival;
        vm->priority = hv->cfg.vms[i].priority;
        vm->mem_words = hv->cfg.vms[i].mem_words;
        vm->first_dispatch = NO_TICK;
        vm->finish = NO_TICK;
        vm->last_cpu = -1;
        vm->tau = 5.0;
        vm->ready_order = NO_TICK;
        vm->page_count = (vm->mem_words + hv->cfg.page_words - 1U) / hv->cfg.page_words;
        log_event(hv, "INFO", "vm", vm->name, "VM_CREATE", "program_loaded_on_init=true");
        if (!cpu_load_program(vm, vm->program_path, error, sizeof(error))) {
            (void)fprintf(stderr, "%s\n", error);
            log_event(hv, "ERROR", "cpu", vm->name, "PROGRAM_REJECTED", error);
            vm_exit(hv, vm, "INVALID_INSTRUCTION");
            continue;
        }
        if (vm->mem_words > SIZE_MAX / sizeof(*vm->backing)) return false;
        bytes = vm->mem_words * sizeof(*vm->backing);
        vm->backing = (int32_t *)calloc(1U, bytes);
        vm->pages = (PageEntry *)calloc(vm->page_count, sizeof(*vm->pages));
        if (vm->backing == NULL || vm->pages == NULL) {
            (void)fprintf(stderr, "Memoria insuficiente ao criar VM %s\n", vm->name);
            return false;
        }
        for (size_t p = 0; p < vm->page_count; ++p) vm->pages[p].frame = -1;
    }
    return true;
}

Vm *vm_find(Hv *hv, const char *name_or_id) {
    char *end = NULL;
    long id;
    if (hv == NULL || name_or_id == NULL) return NULL;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        if (strcmp(hv->vms[i].name, name_or_id) == 0) return &hv->vms[i];
    }
    id = strtol(name_or_id, &end, 10);
    if (end != name_or_id && *end == '\0' && id >= 0 && (size_t)id < hv->vm_count) return &hv->vms[id];
    return NULL;
}
