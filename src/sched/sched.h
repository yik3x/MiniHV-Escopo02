#ifndef MINIHV_SCHED_H
#define MINIHV_SCHED_H
#include "../common.h"

typedef struct SchedPolicy {
    const char *name;
    bool (*init)(Hv *hv);
    bool (*enqueue)(Hv *hv, Vm *vm);
    int (*pick_next)(Hv *hv);
    bool (*on_tick)(Hv *hv, Vm *vm);
    void (*on_block)(Hv *hv, Vm *vm);
    void (*destroy)(Hv *hv);
} SchedPolicy;

const SchedPolicy *sched_policy_create(const char *name);
bool sched_init(Hv *hv);
void sched_destroy(Hv *hv);
bool sched_enqueue(Hv *hv, Vm *vm);
void sched_remove(Hv *hv, Vm *vm);
bool sched_sync_ready(Hv *hv);
int sched_pick_next(Hv *hv);
bool sched_on_tick(Hv *hv, Vm *vm);
void sched_end_burst(Hv *hv, Vm *vm);
#endif
