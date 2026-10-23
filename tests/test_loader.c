#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cpu/loader.h"

int main(void) {
    Vm good = {0};
    Vm bad = {0};
    char error[512] = {0};
    good.mem_words = 256U;
    assert(cpu_load_program(&good, "progs/contador.mv", error, sizeof(error)));
    assert(good.program_count == 6U);
    assert(good.program[0].op == OP_MOV);
    assert(good.program[2].op == OP_OUT);
    assert(good.program[5].op == OP_HALT);
    free(good.program);
    bad.mem_words = 128U;
    assert(!cpu_load_program(&bad, "progs/invalid.mv", error, sizeof(error)));
    assert(strstr(error, "progs/invalid.mv:2:") != NULL);
    (void)puts("test_loader: OK");
    return 0;
}
