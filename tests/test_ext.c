#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/common.h"
#include "../src/hv/hv.h"

int main(void) {
    Hv *hv = hv_create("conf/watchdog-test.conf");
    assert(hv != NULL);
    hv_run(hv);
    assert(strcmp(hv->vms[0].exit_reason, "WATCHDOG") == 0);
    assert(strcmp(hv->vms[1].exit_reason, "HALT") == 0);
    hv_destroy(hv);
    (void)puts("test_ext: OK");
    return 0;
}
