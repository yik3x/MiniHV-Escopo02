#ifndef MINIHV_HV_H
#define MINIHV_HV_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Hv Hv;

/* Public facade: main and cli depend on this API, not on Hv internals. */
Hv *hv_create(const char *config_path);
void hv_destroy(Hv *hv);
bool hv_step(Hv *hv);
void hv_run(Hv *hv);
bool hv_all_done(const Hv *hv);
void hv_report(Hv *hv);
void hv_print_mem(const Hv *hv);
void hv_print_io(const Hv *hv);
void hv_print_stats(const Hv *hv);
bool hv_set_policy(Hv *hv, const char *policy);
void hv_print_ps(const Hv *hv);
bool hv_pause_vm(Hv *hv, const char *name);
bool hv_resume_vm(Hv *hv, const char *name);
bool hv_kill_vm(Hv *hv, const char *name);
uint64_t hv_current_tick(const Hv *hv);
uint64_t hv_max_ticks(const Hv *hv);
bool hv_report_was_printed(const Hv *hv);

#endif
