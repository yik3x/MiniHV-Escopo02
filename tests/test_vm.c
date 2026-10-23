#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/common.h"
#include "../src/hv/hv.h"
#include "../src/vm/vm.h"

int main(void) {
    Hv *hv = hv_create("conf/acceptance-ta01.conf");
    Vm *vm;
    assert(hv != NULL);
    vm = &hv->vms[0];
    assert(vm->state == VM_NEW);
    assert(vm_set_state(hv, vm, VM_READY));
    assert(!vm_set_state(hv, vm, VM_BLOCKED));
    assert(vm->state == VM_READY);
    assert(vm_set_state(hv, vm, VM_RUNNING));
    assert(vm_set_state(hv, vm, VM_BLOCKED));
    assert(vm_set_state(hv, vm, VM_READY));
    vm_exit(hv, vm, "KILLED");
    assert(vm->state == VM_TERMINATED);
    assert(strcmp(vm->exit_reason, "KILLED") == 0);
    hv_destroy(hv);
    (void)puts("test_vm: OK");
    return 0;
}
