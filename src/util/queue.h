#ifndef MINIHV_QUEUE_H
#define MINIHV_QUEUE_H
#include "../common.h"
typedef struct Queue Queue;
typedef int (*QueueCompare)(const void *left, const void *right, void *context);
Queue *queue_create(void);
void queue_destroy(Queue *queue);
bool queue_push(Queue *queue, void *value);
void *queue_pop(Queue *queue);
void *queue_take_best(Queue *queue, QueueCompare compare, void *context);
bool queue_remove(Queue *queue, const void *value);
size_t queue_size(const Queue *queue);
#endif
