#ifndef CORE_STACK_H
#define CORE_STACK_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    size_t count;
    size_t capacity;
    size_t item_size;
    void **items;
} CoreStack;

CoreStack *CoreStack_create(size_t item_size, size_t initial_capacity);
void CoreStack_destroy(CoreStack *stack);
bool CoreStack_push(CoreStack *stack, void *item);
void *CoreStack_pop(CoreStack *stack);
void *CoreStack_peek(const CoreStack *stack);

#endif // CORE_STACK_H
