#include "CoreDictionary.h"
#include <stdlib.h>
#include <string.h>

CoreDictionary *CoreDictionary_create(size_t initial_capacity) {
    CoreDictionary *dict = (CoreDictionary *)malloc(sizeof(CoreDictionary));
    if (dict == NULL) {
        return NULL;
    }
    dict->count = 0;
    dict->capacity = (initial_capacity > 0) ? initial_capacity : 1;
    dict->keys   = (char **)malloc(dict->capacity * sizeof(char *));
    dict->values = (void **)malloc(dict->capacity * sizeof(void *));
    if (dict->keys == NULL || dict->values == NULL) {
        free(dict->keys);
        free(dict->values);
        free(dict);
        return NULL;
    }
    // Initialize to NULL
    for (size_t i = 0; i < dict->capacity; i++) {
        dict->keys[i]   = NULL;
        dict->values[i] = NULL;
    }
    return dict;
}

void CoreDictionary_destroy(CoreDictionary *dict) {
    if (dict == NULL) {
        return;
    }
    // Free all keys; do not free values (caller owns them)
    for (size_t i = 0; i < dict->count; i++) {
        free(dict->keys[i]);
        dict->keys[i] = NULL;
    }
    free(dict->keys);
    free(dict->values);
    free(dict);
}

void *CoreDictionary_put(CoreDictionary *dict, const char *key, void *value) {
    if (dict == NULL || key == NULL) {
        return NULL;
    }
    // Check if key already exists
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->keys[i], key) == 0) {
            // Replace existing value
            void *old_value = dict->values[i];
            free(dict->keys[i]);
            dict->keys[i] = strdup(key);
            if (dict->keys[i] == NULL) {
                // strdup failure: restore old key and return error sentinel
                dict->keys[i] = strdup(key); // attempt again for safety
                return (void *)-1;
            }
            dict->values[i] = value;
            return old_value;
        }
    }
    // New key: expand arrays if needed
    if (dict->count >= dict->capacity) {
        size_t new_capacity = dict->capacity * 2;
        char **new_keys = (char **)realloc(dict->keys, new_capacity * sizeof(char *));
        if (new_keys == NULL) {
            return (void *)-1;
        }
        dict->keys = new_keys;

        void **new_values = (void **)realloc(dict->values, new_capacity * sizeof(void *));
        if (new_values == NULL) {
            return (void *)-1;
        }
        dict->values = new_values;

        // Initialize new slots
        for (size_t j = dict->capacity; j < new_capacity; j++) {
            dict->keys[j]   = NULL;
            dict->values[j] = NULL;
        }
        dict->capacity = new_capacity;
    }
    // Insert new key/value
    dict->keys[dict->count] = strdup(key);
    if (dict->keys[dict->count] == NULL) {
        return (void *)-1;
    }
    dict->values[dict->count] = value;
    dict->count++;
    return NULL; // No previous value
}

void *CoreDictionary_get(const CoreDictionary *dict, const char *key) {
    if (dict == NULL || key == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->keys[i], key) == 0) {
            return dict->values[i];
        }
    }
    return NULL;
}

void *CoreDictionary_remove(CoreDictionary *dict, const char *key) {
    if (dict == NULL || key == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->keys[i], key) == 0) {
            void *old_value = dict->values[i];
            free(dict->keys[i]);

            // Shift remaining entries left
            for (size_t j = i; j + 1 < dict->count; j++) {
                dict->keys[j]   = dict->keys[j + 1];
                dict->values[j] = dict->values[j + 1];
            }
            dict->count--;
            dict->keys[dict->count]   = NULL;
            dict->values[dict->count] = NULL;
            return old_value;
        }
    }
    return NULL;
}

size_t CoreDictionary_size(const CoreDictionary *dict) {
    if (dict == NULL) {
        return 0;
    }
    return dict->count;
}
