#ifndef MINIHV_LOG_H
#define MINIHV_LOG_H
#include "../common.h"
bool log_open(Hv *hv);
void log_close(Hv *hv);
void log_event(Hv *hv, const char *level, const char *module, const char *vm,
               const char *event, const char *detail);
#endif
