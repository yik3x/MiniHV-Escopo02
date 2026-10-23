#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/common.h"
#include "../src/hv/hv.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta01.conf");
    FILE *file;
    char line[512];
    int saw_header = 0;
    int saw_exit = 0;
    assert(hv != NULL);
    hv_run(hv);
    hv_destroy(hv);
    file = fopen("out/acceptance-ta01.csv", "r");
    assert(file != NULL);
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "tick,level,module,vm,event,detail", 33U) == 0) saw_header = 1;
        if (strstr(line, ",VM_EXIT,") != NULL) saw_exit = 1;
    }
    (void)fclose(file);
    assert(saw_header && saw_exit);
    (void)puts("test_log: OK");
    return 0;
}
