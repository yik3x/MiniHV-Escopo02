#include <assert.h>
#include <stdint.h>
#include "../src/util/list.h"
#include "../src/util/queue.h"
#include "../src/util/prng.h"
#include "../src/hv/events.h"

static void count_event(Hv *hv, const HvEvent *event, void *context) {
    int *count = (int *)context;
    assert(hv != NULL && event != NULL && event->tick == 17U);
    (*count)++;
}

static int compare_ints(const void *left, const void *right, void *context) {
    const int *a = (const int *)left;
    const int *b = (const int *)right;
    (void)context;
    return (*a > *b) - (*a < *b);
}

int main(void) {
    int a = 4;
    int b = 2;
    int c = 9;
    List *list = list_create();
    Queue *queue = queue_create();
    Prng first;
    Prng second;
    Hv hv = {0};
    HvEvent event = {0U, "INFO", "test", "vm", "UNIT_EVENT", "test=true", -1, false, false};
    int observed = 0;
    assert(list != NULL && queue != NULL);
    assert(list_append(list, &a));
    assert(list_append(list, &b));
    assert(list_append(list, &c));
    assert(list_size(list) == 3U);
    assert(list_get(list, 0U) == &a);
    assert(list_get(list, 2U) == &c);
    assert(list_get(list, 3U) == NULL);
    assert(list_remove(list, &b));
    assert(!list_remove(list, &b));
    assert(list_size(list) == 2U);
    assert(list_get(list, 1U) == &c);

    assert(queue_push(queue, &a));
    assert(queue_push(queue, &b));
    assert(queue_push(queue, &c));
    assert(queue_size(queue) == 3U);
    assert(queue_take_best(queue, compare_ints, NULL) == &b);
    assert(queue_pop(queue) == &a);
    assert(queue_remove(queue, &c));
    assert(queue_size(queue) == 0U);

    prng_seed(&first, 2026U);
    prng_seed(&second, 2026U);
    assert(prng_next(&first) == UINT32_C(191421147));
    assert(prng_next(&second) == UINT32_C(191421147));
    assert(prng_next(&first) == UINT32_C(783686008));
    assert(prng_next(&second) == UINT32_C(783686008));
    assert(list_size(NULL) == 0U);
    assert(queue_size(NULL) == 0U);

    hv.tick = 17U;
    assert(hv_event_bus_init(&hv));
    assert(hv_observe(&hv, count_event, &observed));
    hv_emit(&hv, &event);
    assert(observed == 1);
    hv_event_bus_destroy(&hv);

    list_destroy(list);
    queue_destroy(queue);
    return 0;
}
