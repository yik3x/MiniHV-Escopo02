#include <assert.h>
#include <stdio.h>
#include "../src/hv/hv.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta01.conf");
    assert(hv != NULL);
    assert(hv_current_tick(hv) == 0U);
    assert(hv_max_ticks(hv) == 500U);
    assert(hv_step(hv));
    assert(hv_current_tick(hv) == 1U);
    assert(!hv_report_was_printed(hv));
    hv_destroy(hv);
    (void)puts("test_hv: OK");
    return 0;
}
