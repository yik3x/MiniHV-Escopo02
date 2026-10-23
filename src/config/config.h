#ifndef MINIHV_CONFIG_H
#define MINIHV_CONFIG_H
#include "../common.h"
void config_defaults(Config *cfg);
bool config_load(Config *cfg, const char *path);
bool config_validate_scope02(const Config *cfg);
#endif
