#ifndef MINIHV_LIST_H
#define MINIHV_LIST_H
#include <stdbool.h>
#include <stddef.h>
typedef struct List List;
List *list_create(void);
void list_destroy(List *list);
bool list_append(List *list, void *value);
void *list_get(const List *list, size_t index);
bool list_remove(List *list, const void *value);
size_t list_size(const List *list);
#endif
