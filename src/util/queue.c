#include "queue.h"

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

struct Queue {
    QueueNode *head;
    QueueNode *tail;
    size_t size;
};

Queue *queue_create(void) {
    return (Queue *)calloc(1U, sizeof(Queue));
}

void queue_destroy(Queue *queue) {
    QueueNode *node;
    if (queue == NULL) return;
    node = queue->head;
    while (node != NULL) {
        QueueNode *next = node->next;
        free(node);
        node = next;
    }
    free(queue);
}

bool queue_push(Queue *queue, void *value) {
    QueueNode *node;
    if (queue == NULL) return false;
    node = (QueueNode *)malloc(sizeof(*node));
    if (node == NULL) return false;
    node->value = value;
    node->next = NULL;
    if (queue->tail == NULL) queue->head = node;
    else queue->tail->next = node;
    queue->tail = node;
    queue->size++;
    return true;
}

void *queue_pop(Queue *queue) {
    QueueNode *node;
    void *value;
    if (queue == NULL || queue->head == NULL) return NULL;
    node = queue->head;
    value = node->value;
    queue->head = node->next;
    if (queue->head == NULL) queue->tail = NULL;
    queue->size--;
    free(node);
    return value;
}

void *queue_take_best(Queue *queue, QueueCompare compare, void *context) {
    QueueNode *node;
    QueueNode *previous = NULL;
    QueueNode *best = NULL;
    QueueNode *best_previous = NULL;
    if (queue == NULL || queue->head == NULL) return NULL;
    if (compare == NULL) return queue_pop(queue);
    for (node = queue->head; node != NULL; node = node->next) {
        if (best == NULL || compare(node->value, best->value, context) < 0) {
            best = node;
            best_previous = previous;
        }
        previous = node;
    }
    if (best_previous == NULL) queue->head = best->next;
    else best_previous->next = best->next;
    if (queue->tail == best) queue->tail = best_previous;
    queue->size--;
    void *value = best->value;
    free(best);
    return value;
}

bool queue_remove(Queue *queue, const void *value) {
    QueueNode *node;
    QueueNode *previous = NULL;
    if (queue == NULL) return false;
    for (node = queue->head; node != NULL; node = node->next) {
        if (node->value == value) {
            if (previous == NULL) queue->head = node->next;
            else previous->next = node->next;
            if (queue->tail == node) queue->tail = previous;
            queue->size--;
            free(node);
            return true;
        }
        previous = node;
    }
    return false;
}

size_t queue_size(const Queue *queue) {
    return queue == NULL ? 0U : queue->size;
}
