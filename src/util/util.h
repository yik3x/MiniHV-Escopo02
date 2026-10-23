#ifndef MINIHV_UTIL_H
#define MINIHV_UTIL_H
#include "../common.h"
void util_trim(char *text);
void hv_emit_instruction(Hv *hv, Vm *vm, bool io_instruction);
bool util_parse_i32(const char *text, int32_t *value);
bool util_parse_u64(const char *text, uint64_t *value);
bool util_parse_size(const char *text, size_t *value);
#endif
