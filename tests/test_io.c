#include <assert.h>
#include <stdio.h>
#include "../src/common.h"
#include "../src/hv/hv.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta03.conf");
    assert(hv != NULL);
    hv_run(hv);
    assert(hv->devices[DEV_CONSOLE].requests_served > 0U);
    assert(hv->devices[DEV_DISK].requests_served > 0U);
    assert(hv->devices[DEV_NET].requests_served > 0U);
    assert(hv->io_interrupt_handler != NULL);
    hv_destroy(hv);
    (void)puts("test_io: OK");
    return 0;
}
