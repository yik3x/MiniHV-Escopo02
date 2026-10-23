#include <assert.h>
#include <stdio.h>
#include "../src/common.h"
#include "../src/hv/hv.h"
#include "../src/io/io.h"
#include "../src/mem/mem.h"
#include "../src/vm/vm.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta01.conf");
    Vm *vm;
    assert(hv != NULL);
    vm = &hv->vms[0];
    assert(vm_set_state(hv, vm, VM_READY));
    assert(vm_set_state(hv, vm, VM_RUNNING));
    assert(mem_fetch_instruction(hv, vm, 0) == MEM_PENDING);
    assert(vm->state == VM_BLOCKED);
    assert(vm->page_faults == 1U);
    assert(mem_used_frames(hv) == 1U);
    io_tick(hv);
    io_tick(hv);
    io_tick(hv);
    assert(vm->state == VM_READY);
    assert(mem_fetch_instruction(hv, vm, 0) == MEM_OK);
    mem_release_vm(hv, vm);
    assert(mem_used_frames(hv) == 0U);
    hv_destroy(hv);
    (void)puts("test_mem: OK");
    return 0;
}
