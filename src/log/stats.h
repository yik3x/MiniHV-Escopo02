#ifndef MINIHV_STATS_H
#define MINIHV_STATS_H
#include "../common.h"
bool stats_init(Hv *hv);
void stats_print(const Hv *hv);
void stats_report(Hv *hv);
#endif
