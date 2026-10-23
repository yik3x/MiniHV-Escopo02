#include <assert.h>
#include <stdio.h>
#include "../src/common.h"
#include "../src/hv/hv.h"
#include "../src/sched/sched.h"
#include "../src/vm/vm.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta02.conf");
    assert(hv != NULL);
    assert(sched_policy_create("SJF") != NULL);
    assert(sched_policy_create("FCFS") != NULL);
    assert(sched_policy_create("SRTF") == NULL);
    hv->vms[0].tau = 4.0;
    hv->vms[1].tau = 2.0;
    hv->vms[2].tau = 2.0;
    hv->vms[3].tau = 6.0;
    for (size_t i = 0; i < 4U; ++i) {
        assert(vm_set_state(hv, &hv->vms[i], VM_READY));
        assert(sched_enqueue(hv, &hv->vms[i]));
    }
    assert(sched_pick_next(hv) == 1);
    assert(sched_pick_next(hv) == 2);
    assert(sched_pick_next(hv) == 0);
    assert(sched_pick_next(hv) == 3);
    hv_destroy(hv);
    (void)puts("test_sched: OK");
    return 0;
}
