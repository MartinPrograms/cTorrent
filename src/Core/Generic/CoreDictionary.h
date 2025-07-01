#ifndef CORE_DICTIONARY_H
#define CORE_DICTIONARY_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t count;
    size_t capacity;
    char   **keys;
    void   **values;
} CoreDictionary;

CoreDictionary *CoreDictionary_create(size_t initial_capacity);
void CoreDictionary_destroy(CoreDictionary *dict);
void *CoreDictionary_put(CoreDictionary *dict, const char *key, void *value);
void *CoreDictionary_get(const CoreDictionary *dict, const char *key);
void *CoreDictionary_remove(CoreDictionary *dict, const char *key);
size_t CoreDictionary_size(const CoreDictionary *dict);

#endif // CORE_DICTIONARY_H
