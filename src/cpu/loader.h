#ifndef MINIHV_LOADER_H
#define MINIHV_LOADER_H
#include "../common.h"
bool cpu_load_program(Vm *vm, const char *path, char *error, size_t error_capacity);
#endif
