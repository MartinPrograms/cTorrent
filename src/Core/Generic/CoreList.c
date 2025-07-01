#include "CoreList.h"
#include <stdlib.h>
#include <string.h>

CoreList *CoreList_create(size_t initial_capacity) {
    CoreList *list = (CoreList *)malloc(sizeof(CoreList));
    if (list == NULL) {
        return NULL;
    }
    list->count = 0;
    list->capacity = (initial_capacity > 0) ? initial_capacity : 1;
    list->items = (void **)malloc(list->capacity * sizeof(void *));
    if (list->items == NULL) {
        free(list);
        return NULL;
    }
    // Initialize pointers to NULL
    for (size_t i = 0; i < list->capacity; i++) {
        list->items[i] = NULL;
    }
    return list;
}

void CoreList_destroy(CoreList *list) {
    if (list == NULL) {
        return;
    }
    free(list->items);
    free(list);
}

bool CoreList_append(CoreList *list, void *item) {
    if (list == NULL || item == NULL) {
        return false;
    }
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        void **new_items = (void **)realloc(list->items, new_capacity * sizeof(void *));
        if (new_items == NULL) {
            return false;
        }
        // Initialize any newly allocated slots to NULL
        for (size_t i = list->capacity; i < new_capacity; i++) {
            new_items[i] = NULL;
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = item;
    return true;
}

void *CoreList_get(const CoreList *list, size_t index) {
    if (list == NULL || index >= list->count) {
        return NULL;
    }
    return list->items[index];
}

void *CoreList_remove(CoreList *list, size_t index) {
    if (list == NULL || index >= list->count) {
        return NULL;
    }
    void *removed = list->items[index];
    // Shift subsequent elements left
    for (size_t i = index; i + 1 < list->count; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;
    list->items[list->count] = NULL; // Clear last slot
    return removed;
}

size_t CoreList_size(const CoreList *list) {
    if (list == NULL) {
        return 0;
    }
    return list->count;
}
