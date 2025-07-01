#ifndef CORE_LIST_H
#define CORE_LIST_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t count;
    size_t capacity;
    void **items;
} CoreList;

CoreList *CoreList_create(size_t initial_capacity);
void CoreList_destroy(CoreList *list);
bool CoreList_append(CoreList *list, void *item);
void *CoreList_get(const CoreList *list, size_t index);
void *CoreList_remove(CoreList *list, size_t index);
size_t CoreList_size(const CoreList *list);

#endif // CORE_LIST_H
