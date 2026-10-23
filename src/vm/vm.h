#ifndef MINIHV_VM_H
#define MINIHV_VM_H
#include "../common.h"
bool vm_init_all(Hv *hv);
bool vm_set_state(Hv *hv, Vm *vm, VmState next);
void vm_exit(Hv *hv, Vm *vm, const char *reason);
Vm *vm_find(Hv *hv, const char *name_or_id);
#endif
