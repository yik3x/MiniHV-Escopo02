#include "hv.h"
#include "../common.h"
#include "events.h"
#include "../config/config.h"
#include "../cpu/cpu.h"
#include "../ext/ext.h"
#include "../io/io.h"
#include "../log/log.h"
#include "../log/stats.h"
#include "../mem/mem.h"
#include "../sched/sched.h"
#include "../vm/vm.h"

static size_t active_vm_count(const Hv *hv) {
    size_t count = 0U;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        VmState state = hv->vms[i].state;
        if (state != VM_NEW && state != VM_TERMINATED) count++;
    }
    return count;
}

/* RF-IO-02: device completions are reported upward through this HV callback. */
static void hv_handle_io_interrupt(Hv *hv, int vm_id, DeviceType device,
                                   size_t page, void *context) {
    Vm *vm;
    (void)context;
    if (hv == NULL || vm_id < 0 || (size_t)vm_id >= hv->vm_count) return;
    vm = &hv->vms[vm_id];
    if (device == DEV_SWAP && vm->state == VM_BLOCKED &&
        !mem_complete_page_fault(hv, vm, page)) {
        log_event(hv, "ERROR", "mem", vm->name, "PAGE_IN_FAILED", "invalid_pending_page=true");
        vm_exit(hv, vm, "KILLED");
        mem_release_vm(hv, vm);
        return;
    }
    if (vm->state == VM_BLOCKED && vm_set_state(hv, vm, VM_READY)) {
        vm->ready_order = hv->tick * (uint64_t)MAX_VMS + (uint64_t)vm->id;
    }
}

static void admit_vms(Hv *hv) {
    while (active_vm_count(hv) < (size_t)hv->cfg.max_vms) {
        int selected = -1;
        for (size_t i = 0; i < hv->vm_count; ++i) {
            Vm *candidate = &hv->vms[i];
            if (candidate->state != VM_NEW || candidate->arrival > hv->tick) continue;
            if (selected < 0 || candidate->arrival < hv->vms[selected].arrival ||
                (candidate->arrival == hv->vms[selected].arrival && candidate->id < hv->vms[selected].id)) {
                selected = (int)i;
            }
        }
        if (selected < 0) break;
        Vm *vm = &hv->vms[selected];
        if (vm_set_state(hv, vm, VM_READY)) {
            if (!sched_enqueue(hv, vm)) {
                vm_exit(hv, vm, "KILLED");
                mem_release_vm(hv, vm);
                continue;
            }
            vm->admitted = true;
            log_event(hv, "INFO", "vm", vm->name, "VM_ADMIT", "state=READY");
        } else break;
    }
}

bool hv_all_done(const Hv *hv) {
    if (hv == NULL) return true;
    for (size_t i = 0; i < hv->vm_count; ++i) if (hv->vms[i].state != VM_TERMINATED) return false;
    return true;
}

Hv *hv_create(const char *config_path) {
    Hv *hv = (Hv *)calloc(1U, sizeof(*hv));
    const char *path = config_path == NULL ? "conf/hv.conf" : config_path;
    if (hv == NULL) { (void)fprintf(stderr, "Memoria insuficiente ao criar o MiniHV\n"); return NULL; }
    hv->running_vm = -1;
    hv->last_dispatched_vm = -1;
    if (!config_load(&hv->cfg, path)) { free(hv); return NULL; }
    if (!hv_event_bus_init(hv)) { free(hv); return NULL; }
    prng_seed(&hv->prng, hv->cfg.seed);
    if (!log_open(hv)) { hv_destroy(hv); return NULL; }
    if (!stats_init(hv)) { hv_destroy(hv); return NULL; }
    log_event(hv, "INFO", "hv", "-", "HV_INIT", "scope=02");
    if (!mem_init(hv)) { hv_destroy(hv); return NULL; }
    if (!io_init(hv)) { hv_destroy(hv); return NULL; }
    io_set_interrupt_handler(hv, hv_handle_io_interrupt, NULL);
    if (!sched_init(hv)) { hv_destroy(hv); return NULL; }
    if (!vm_init_all(hv)) { hv_destroy(hv); return NULL; }
    if (!ext_init(hv)) { hv_destroy(hv); return NULL; }
    hv->initialized = true;
    return hv;
}

void hv_destroy(Hv *hv) {
    if (hv == NULL) return;
    log_event(hv, "INFO", "hv", "-", "HV_DESTROY", "resources_released=true");
    io_destroy(hv);
    sched_destroy(hv);
    for (size_t i = 0; i < MAX_VMS; ++i) {
        free(hv->vms[i].program);
        free(hv->vms[i].backing);
        free(hv->vms[i].pages);
        hv->vms[i].program = NULL;
        hv->vms[i].backing = NULL;
        hv->vms[i].pages = NULL;
    }
    mem_destroy(hv);
    log_close(hv);
    hv_event_bus_destroy(hv);
    free(hv);
}

bool hv_step(Hv *hv) {
    if (hv == NULL || !hv->initialized || hv_all_done(hv) || hv->tick >= hv->cfg.max_ticks) return false;
    /* RF-HV-02: admit, device progress/interrupts, dispatch, execute, stats, clock. */
    admit_vms(hv);
    io_tick(hv);
    if (!sched_sync_ready(hv)) {
        for (size_t i = 0; i < hv->vm_count; ++i) {
            Vm *vm = &hv->vms[i];
            if (vm->state == VM_READY && !vm->queued) {
                io_cancel_vm(hv, vm->id);
                vm_exit(hv, vm, "KILLED");
                mem_release_vm(hv, vm);
            }
        }
    }
    if (hv->running_vm >= 0 && hv->vms[hv->running_vm].state != VM_RUNNING) hv->running_vm = -1;
    if (hv->running_vm < 0) {
        int selected = sched_pick_next(hv);
        if (selected >= 0) {
            Vm *vm = &hv->vms[selected];
            if (vm_set_state(hv, vm, VM_RUNNING)) {
                char detail[128];
                if (vm->first_dispatch == NO_TICK) vm->first_dispatch = hv->tick;
                if (hv->last_dispatched_vm >= 0 && hv->last_dispatched_vm != selected) {
                    hv->context_switches++;
                    (void)snprintf(detail, sizeof(detail), "from=%.48s to=%.48s cpu=0",
                                   hv->vms[hv->last_dispatched_vm].name, vm->name);
                    log_event(hv, "INFO", "cpu", vm->name, "CTX_SWITCH", detail);
                }
                hv->last_dispatched_vm = selected;
                vm->last_cpu = 0;
                vm->burst_ticks = 0U;
                hv->running_vm = selected;
                (void)snprintf(detail, sizeof(detail), "cpu=0 tau=%.3f policy=%s", vm->tau, hv->cfg.policy);
                log_event(hv, "INFO", "sched", vm->name, "DISPATCH", detail);
            }
        }
    }
    if (hv->running_vm >= 0) {
        Vm *vm = &hv->vms[hv->running_vm];
        bool preempted = false;
        if (vm->state == VM_RUNNING) cpu_step(hv, vm);
        if (vm->state == VM_RUNNING && sched_on_tick(hv, vm)) {
            log_event(hv, "INFO", "sched", vm->name, "PREEMPT", "reason=policy cpu=0");
            sched_end_burst(hv, vm);
            preempted = vm_set_state(hv, vm, VM_READY);
            if (preempted && !sched_enqueue(hv, vm)) {
                vm_exit(hv, vm, "KILLED");
                mem_release_vm(hv, vm);
            }
        }
        if (vm->state != VM_RUNNING) {
            if (!preempted) sched_end_burst(hv, vm);
            if (vm->state == VM_TERMINATED) {
                io_cancel_vm(hv, vm->id);
                sched_remove(hv, vm);
                mem_release_vm(hv, vm);
            }
            hv->running_vm = -1;
        }
    }
    for (size_t i = 0; i < hv->vm_count; ++i) if (hv->vms[i].state == VM_READY) hv->vms[i].wait_ticks++;
    hv->occupancy_sum += (uint64_t)mem_used_frames(hv);
    hv->occupancy_samples++;
    hv->tick++;
    return true;
}

void hv_run(Hv *hv) {
    if (hv == NULL) return;
    while (hv->tick < hv->cfg.max_ticks && !hv_all_done(hv)) {
        if (!hv_step(hv)) break;
    }
    if (!hv_all_done(hv) && hv->tick >= hv->cfg.max_ticks) {
        for (size_t i = 0; i < hv->vm_count; ++i) {
            if (hv->vms[i].state != VM_TERMINATED) {
                io_cancel_vm(hv, (int)i);
                if (hv->running_vm == (int)i) hv->running_vm = -1;
                sched_end_burst(hv, &hv->vms[i]);
                vm_exit(hv, &hv->vms[i], "MAX_TICKS");
                mem_release_vm(hv, &hv->vms[i]);
            }
        }
    }
}

uint64_t hv_current_tick(const Hv *hv) {
    return hv == NULL ? 0U : hv->tick;
}

uint64_t hv_max_ticks(const Hv *hv) {
    return hv == NULL ? 0U : hv->cfg.max_ticks;
}

bool hv_report_was_printed(const Hv *hv) {
    return hv != NULL && hv->report_printed;
}

void hv_print_ps(const Hv *hv) {
    if (hv == NULL) return;
    (void)printf("ID  VM                  ESTADO       PC       CHEGADA  CPU  TAU\n");
    for (size_t i = 0; i < hv->vm_count; ++i) {
        const Vm *vm = &hv->vms[i];
        (void)printf("%-3d %-19s %-12s %-8lld %-8llu %-4llu %.2f",
            vm->id, vm->name,
            vm->state == VM_TERMINATED && vm->exit_reason[0] != '\0' ? vm->exit_reason : vm_state_name(vm->state),
            (long long)vm->pc, (unsigned long long)vm->arrival,
            (unsigned long long)vm->cpu_ticks, vm->tau);
        if (vm->state == VM_TERMINATED) (void)printf(" fim=%llu", (unsigned long long)vm->finish);
        (void)printf("\n");
    }
}

void hv_print_stats(const Hv *hv) { stats_print(hv); }

void hv_print_mem(const Hv *hv) { mem_print_status(hv); }

void hv_print_io(const Hv *hv) { io_print_status(hv); }

static bool check_vm_argument(Hv *hv, const char *name, Vm **vm) {
    *vm = vm_find(hv, name);
    if (*vm == NULL) { (void)fprintf(stderr, "VM nao encontrada: %s\n", name); return false; }
    return true;
}

bool hv_pause_vm(Hv *hv, const char *name) {
    Vm *vm;
    if (hv == NULL || !check_vm_argument(hv, name, &vm)) return false;
    if (vm->state != VM_READY && vm->state != VM_RUNNING) {
        (void)fprintf(stderr, "pause recusado: a VM precisa estar em READY ou RUNNING\n"); return false;
    }
    if (vm->state == VM_RUNNING) {
        sched_end_burst(hv, vm);
        hv->running_vm = -1;
    }
    sched_remove(hv, vm);
    return vm_set_state(hv, vm, VM_PAUSED);
}

bool hv_resume_vm(Hv *hv, const char *name) {
    Vm *vm;
    if (hv == NULL || !check_vm_argument(hv, name, &vm)) return false;
    if (vm->state != VM_PAUSED) { (void)fprintf(stderr, "resume recusado: a VM precisa estar em PAUSED\n"); return false; }
    if (!vm_set_state(hv, vm, VM_READY)) return false;
    if (!sched_enqueue(hv, vm)) {
        (void)vm_set_state(hv, vm, VM_PAUSED);
        return false;
    }
    return true;
}

bool hv_kill_vm(Hv *hv, const char *name) {
    Vm *vm;
    if (hv == NULL || !check_vm_argument(hv, name, &vm)) return false;
    if (vm->state == VM_TERMINATED) { (void)fprintf(stderr, "A VM ja terminou\n"); return false; }
    if (hv->running_vm == vm->id) hv->running_vm = -1;
    sched_remove(hv, vm);
    sched_end_burst(hv, vm);
    io_cancel_vm(hv, vm->id);
    vm_exit(hv, vm, "KILLED");
    mem_release_vm(hv, vm);
    return true;
}

void hv_report(Hv *hv) { stats_report(hv); }

bool hv_set_policy(Hv *hv, const char *policy) {
    char previous[sizeof(hv->cfg.policy)];
    if (hv == NULL || policy == NULL ||
        (strcmp(policy, "SJF") != 0 && strcmp(policy, "FCFS") != 0)) return false;
    (void)snprintf(previous, sizeof(previous), "%s", hv->cfg.policy);
    sched_destroy(hv);
    (void)snprintf(hv->cfg.policy, sizeof(hv->cfg.policy), "%s", policy);
    if (sched_init(hv)) return true;
    (void)snprintf(hv->cfg.policy, sizeof(hv->cfg.policy), "%s", previous);
    return sched_init(hv);
}
