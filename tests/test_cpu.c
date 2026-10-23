#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/common.h"
#include "../src/hv/hv.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta01.conf");
    assert(hv != NULL);
    hv_run(hv);
    assert(hv->vms[0].state == VM_TERMINATED);
    assert(strcmp(hv->vms[0].exit_reason, "HALT") == 0);
    assert(hv->vms[0].cpu_ticks > 0U);
    assert(hv->vms[0].program_count == 6U);
    hv_destroy(hv);
    (void)puts("test_cpu: OK");
    return 0;
}
