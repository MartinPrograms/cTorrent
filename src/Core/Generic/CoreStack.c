#include "CoreStack.h"

CoreStack * CoreStack_create(size_t item_size, size_t initial_capacity) {
    CoreStack *stack = (CoreStack *)malloc(sizeof(CoreStack));
    if (stack == NULL) {
        return NULL; // Memory allocation failed
    }
    stack->count = 0;
    stack->capacity = initial_capacity > 0 ? initial_capacity : 1; // Ensure at least 1 capacity
    stack->item_size = item_size;
    stack->items = (void **)malloc(stack->capacity * sizeof(void *));
    if (stack->items == NULL) {
        free(stack);
        return NULL; // Memory allocation failed
    }

    // Initialize each item pointer to NULL
    for (size_t i = 0; i < stack->capacity; i++) {
        stack->items[i] = malloc(item_size);
        if (stack->items[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free(stack->items[j]);
            }
            free(stack->items);
            free(stack);
            return NULL; // Memory allocation failed
        }
    }

    return stack;
}

void CoreStack_destroy(CoreStack *stack) {
    if (stack == NULL) {
        return; // Nothing to destroy
    }

    for (size_t i = 0; i < stack->count; i++) {
        if (stack->items[i] != NULL) {
            free(stack->items[i]); // Free each item
        }
    }

    free(stack->items);
    free(stack);
}

bool CoreStack_push(CoreStack *stack, void *item) {
    if (stack == NULL || item == NULL) {
        return false; // Nothing to push
    }

    if (stack->count >= stack->capacity) {
        // Resize the stack if necessary
        size_t new_capacity = stack->capacity * 2;
        void **new_items = (void **)realloc(stack->items, new_capacity * sizeof(void *));
        if (new_items == NULL) {
            return false;
        }

        stack->items = new_items;
        stack->capacity = new_capacity;
    }

    stack->items[stack->count++] = item;
    return true; // Successfully pushed the item
}

void * CoreStack_pop(CoreStack *stack) {
    if (stack == NULL || stack->count == 0) {
        return NULL; // Nothing to pop
    }

    void *item = stack->items[--stack->count];
    stack->items[stack->count] = NULL; // Clear the popped item
    return item; // Return the popped item
}

void * CoreStack_peek(const CoreStack *stack) {
    if (stack == NULL || stack->count == 0) {
        return NULL; // Nothing to peek
    }

    return stack->items[stack->count - 1]; // Return the last item without removing it
}
