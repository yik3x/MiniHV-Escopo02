#ifndef MINIHV_MEM_H
#define MINIHV_MEM_H
#include "../common.h"
typedef enum { MEM_OK, MEM_PENDING, MEM_ERROR } MemResult;
typedef struct MemPolicy {
    const char *name;
    size_t (*choose_victim)(Hv *hv);
} MemPolicy;
const MemPolicy *mem_policy_create(const char *algorithm, const char *scope);
bool mem_init(Hv *hv);
void mem_destroy(Hv *hv);
void mem_release_vm(Hv *hv, Vm *vm);
bool mem_complete_page_fault(Hv *hv, Vm *vm, size_t page);
MemResult mem_fetch_instruction(Hv *hv, Vm *vm, int64_t address);
MemResult mem_read_word(Hv *hv, Vm *vm, int64_t address, int32_t *value);
MemResult mem_write_word(Hv *hv, Vm *vm, int64_t address, int32_t value);
void mem_print_status(const Hv *hv);
size_t mem_used_frames(const Hv *hv);
#endif
