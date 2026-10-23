#include "list.h"
#include <stdlib.h>

typedef struct ListNode {
    void *value;
    struct ListNode *next;
} ListNode;

struct List {
    ListNode *head;
    ListNode *tail;
    size_t size;
};

List *list_create(void) {
    return (List *)calloc(1U, sizeof(List));
}

void list_destroy(List *list) {
    ListNode *node;
    if (list == NULL) return;
    node = list->head;
    while (node != NULL) {
        ListNode *next = node->next;
        free(node);
        node = next;
    }
    free(list);
}

bool list_append(List *list, void *value) {
    ListNode *node;
    if (list == NULL) return false;
    node = (ListNode *)malloc(sizeof(*node));
    if (node == NULL) return false;
    node->value = value;
    node->next = NULL;
    if (list->tail == NULL) list->head = node;
    else list->tail->next = node;
    list->tail = node;
    list->size++;
    return true;
}

void *list_get(const List *list, size_t index) {
    ListNode *node;
    if (list == NULL || index >= list->size) return NULL;
    node = list->head;
    while (index-- > 0U) node = node->next;
    return node->value;
}

bool list_remove(List *list, const void *value) {
    ListNode *node;
    ListNode *previous = NULL;
    if (list == NULL) return false;
    for (node = list->head; node != NULL; node = node->next) {
        if (node->value == value) {
            if (previous == NULL) list->head = node->next;
            else previous->next = node->next;
            if (list->tail == node) list->tail = previous;
            list->size--;
            free(node);
            return true;
        }
        previous = node;
    }
    return false;
}

size_t list_size(const List *list) {
    return list == NULL ? 0U : list->size;
}
