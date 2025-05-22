#ifndef CORESTRING_H
#define CORESTRING_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *str;
    size_t length;
} CoreString;

CoreString *CoreString_create(const char *str);
void CoreString_destroy(CoreString *coreString);

void CoreString_append(CoreString *coreString, const char *str);
void CoreString_prepend(CoreString *coreString, const char *str);
void CoreString_insert(CoreString *coreString, size_t index, const char *str);
void CoreString_remove(CoreString *coreString, size_t index, size_t length);
void CoreString_replace(CoreString *coreString, size_t index, size_t length, const char *str);
void CoreString_clear(CoreString *coreString);

#endif //CORESTRING_H
