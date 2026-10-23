#include "ext.h"
#include "../hv/events.h"
#include "../log/log.h"
#include "../vm/vm.h"

static void watchdog_observer(Hv *hv, const HvEvent *event, void *context) {
    Vm *vm;
    char detail[128];
    uint64_t limit;
    (void)context;
    if (hv == NULL || event == NULL || !event->instruction || event->vm_id < 0 ||
        (size_t)event->vm_id >= hv->vm_count || !hv->cfg.ext_enabled) return;
    vm = &hv->vms[event->vm_id];
    if (vm->state == VM_TERMINATED) return;
    limit = hv->cfg.watchdog_limit;
    if (event->io_instruction) {
        vm->cpu_since_io = 0U;
        vm->watchdog_warned = false;
        return;
    }
    vm->cpu_since_io++;
    if (!vm->watchdog_warned && vm->cpu_since_io >= limit) {
        (void)snprintf(detail, sizeof(detail), "cpu_ticks_since_io=%llu limit=%llu",
                       (unsigned long long)vm->cpu_since_io, (unsigned long long)limit);
        log_event(hv, "WARN", "ext", vm->name, "WATCHDOG_WARN", detail);
        vm->watchdog_warned = true;
    }
    if (vm->cpu_since_io >= limit * 2U) {
        (void)snprintf(detail, sizeof(detail), "cpu_ticks_since_io=%llu limit=%llu",
                       (unsigned long long)vm->cpu_since_io, (unsigned long long)limit);
        log_event(hv, "WARN", "ext", vm->name, "WATCHDOG", detail);
        vm_exit(hv, vm, "WATCHDOG");
    }
}

bool ext_init(Hv *hv) {
    if (hv == NULL) return false;
    if (!hv->cfg.ext_enabled) return true;
    return hv_observe(hv, watchdog_observer, NULL);
}
