#include "sched.h"
#include "../util/queue.h"
#include "../log/log.h"
#include "../vm/vm.h"

static bool ready_queue_init(Hv *hv) {
    if (hv == NULL) return false;
    hv->ready_queue = queue_create();
    return hv->ready_queue != NULL;
}

static bool enqueue_common(Hv *hv, Vm *vm) {
    if (hv == NULL || vm == NULL || vm->state != VM_READY) return false;
    if (vm->queued) return true;
    if (hv->ready_queue == NULL || !queue_push(hv->ready_queue, vm)) return false;
    vm->ready_order = hv->ready_sequence++;
    vm->queued = true;
    return true;
}

static void destroy_common(Hv *hv) {
    if (hv == NULL) return;
    queue_destroy(hv->ready_queue);
    hv->ready_queue = NULL;
    for (size_t i = 0; i < hv->vm_count; ++i) hv->vms[i].queued = false;
}

static int compare_sjf(const void *left, const void *right, void *context) {
    const Vm *a = (const Vm *)left;
    const Vm *b = (const Vm *)right;
    (void)context;
    if (a->tau < b->tau) return -1;
    if (a->tau > b->tau) return 1;
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static int compare_fcfs(const void *left, const void *right, void *context) {
    const Vm *a = (const Vm *)left;
    const Vm *b = (const Vm *)right;
    (void)context;
    if (a->ready_order < b->ready_order) return -1;
    if (a->ready_order > b->ready_order) return 1;
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static int pick_best(Hv *hv, QueueCompare compare) {
    Vm *selected;
    if (hv == NULL || hv->ready_queue == NULL) return -1;
    selected = (Vm *)queue_take_best(hv->ready_queue, compare, hv);
    if (selected == NULL) return -1;
    selected->queued = false;
    return selected->id;
}

static int pick_sjf(Hv *hv) { return pick_best(hv, compare_sjf); }
static int pick_fcfs(Hv *hv) { return pick_best(hv, compare_fcfs); }

static bool no_preemption(Hv *hv, Vm *vm) {
    (void)hv;
    (void)vm;
    return false;
}

static void end_burst_sjf(Hv *hv, Vm *vm) {
    (void)hv;
    if (vm == NULL) return;
    if (vm->burst_ticks > 0U) vm->tau = ((double)vm->burst_ticks + vm->tau) / 2.0;
    vm->burst_ticks = 0U;
}

static void end_burst_fcfs(Hv *hv, Vm *vm) {
    (void)hv;
    if (vm != NULL) vm->burst_ticks = 0U;
}

static const SchedPolicy SJF_POLICY = {
    "SJF", ready_queue_init, enqueue_common, pick_sjf, no_preemption, end_burst_sjf, destroy_common
};
static const SchedPolicy FCFS_POLICY = {
    "FCFS", ready_queue_init, enqueue_common, pick_fcfs, no_preemption, end_burst_fcfs, destroy_common
};

const SchedPolicy *sched_policy_create(const char *name) {
    if (name != NULL && strcmp(name, "SJF") == 0) return &SJF_POLICY;
    if (name != NULL && strcmp(name, "FCFS") == 0) return &FCFS_POLICY;
    return NULL;
}

bool sched_init(Hv *hv) {
    if (hv == NULL) return false;
    hv->sched_policy_impl = sched_policy_create(hv->cfg.policy);
    if (hv->sched_policy_impl == NULL) {
        (void)fprintf(stderr, "Politica de escalonamento nao suportada: %s\n", hv->cfg.policy);
        return false;
    }
    if (!hv->sched_policy_impl->init(hv)) {
        (void)fprintf(stderr, "Nao foi possivel inicializar a fila de VMs prontas\n");
        return false;
    }
    return true;
}

void sched_destroy(Hv *hv) {
    if (hv == NULL || hv->sched_policy_impl == NULL) return;
    hv->sched_policy_impl->destroy(hv);
    hv->sched_policy_impl = NULL;
}

bool sched_enqueue(Hv *hv, Vm *vm) {
    if (hv == NULL || vm == NULL || hv->sched_policy_impl == NULL) return false;
    if (!hv->sched_policy_impl->enqueue(hv, vm)) {
        log_event(hv, "ERROR", "sched", vm->name, "READY_QUEUE_FAILURE", "enqueue_failed=true");
        return false;
    }
    return true;
}

void sched_remove(Hv *hv, Vm *vm) {
    if (hv == NULL || vm == NULL || hv->ready_queue == NULL || !vm->queued) return;
    (void)queue_remove(hv->ready_queue, vm);
    vm->queued = false;
}

bool sched_sync_ready(Hv *hv) {
    if (hv == NULL) return false;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        Vm *vm = &hv->vms[i];
        if (vm->state == VM_READY && !vm->queued && !sched_enqueue(hv, vm)) return false;
    }
    return true;
}

int sched_pick_next(Hv *hv) {
    if (hv == NULL || hv->sched_policy_impl == NULL) return -1;
    return hv->sched_policy_impl->pick_next(hv);
}

bool sched_on_tick(Hv *hv, Vm *vm) {
    return hv != NULL && vm != NULL && hv->sched_policy_impl != NULL &&
           hv->sched_policy_impl->on_tick(hv, vm);
}

void sched_end_burst(Hv *hv, Vm *vm) {
    if (hv == NULL || vm == NULL || hv->sched_policy_impl == NULL) return;
    hv->sched_policy_impl->on_block(hv, vm);
}
