#include "events.h"
#include "../util/list.h"

typedef struct {
    HvEventCallback callback;
    void *context;
} HvObserver;

bool hv_event_bus_init(Hv *hv) {
    if (hv == NULL) return false;
    hv->observers = list_create();
    return hv->observers != NULL;
}

void hv_event_bus_destroy(Hv *hv) {
    if (hv == NULL || hv->observers == NULL) return;
    for (size_t i = 0; i < list_size(hv->observers); ++i) free(list_get(hv->observers, i));
    list_destroy(hv->observers);
    hv->observers = NULL;
}

bool hv_observe(Hv *hv, HvEventCallback callback, void *context) {
    HvObserver *observer;
    if (hv == NULL || hv->observers == NULL || callback == NULL) return false;
    observer = (HvObserver *)malloc(sizeof(*observer));
    if (observer == NULL) return false;
    observer->callback = callback;
    observer->context = context;
    if (!list_append(hv->observers, observer)) {
        free(observer);
        return false;
    }
    return true;
}

void hv_emit(Hv *hv, const HvEvent *event) {
    HvEvent current;
    if (hv == NULL || event == NULL || hv->observers == NULL) return;
    current = *event;
    current.tick = hv->tick;
    for (size_t i = 0; i < list_size(hv->observers); ++i) {
        HvObserver *observer = (HvObserver *)list_get(hv->observers, i);
        if (observer != NULL) observer->callback(hv, &current, observer->context);
    }
}
