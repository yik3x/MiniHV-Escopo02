#ifndef MINIHV_EVENTS_H
#define MINIHV_EVENTS_H
#include "../common.h"

typedef struct HvEvent {
    uint64_t tick;
    const char *level;
    const char *module;
    const char *vm;
    const char *event;
    const char *detail;
    int vm_id;
    bool instruction;
    bool io_instruction;
} HvEvent;

typedef void (*HvEventCallback)(Hv *hv, const HvEvent *event, void *context);

bool hv_event_bus_init(Hv *hv);
void hv_event_bus_destroy(Hv *hv);
bool hv_observe(Hv *hv, HvEventCallback callback, void *context);
void hv_emit(Hv *hv, const HvEvent *event);
#endif
