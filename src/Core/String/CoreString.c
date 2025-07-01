#include "CoreString.h"

CoreString *CoreString_create(const char *str){
    CoreString *coreString = (CoreString *)malloc(sizeof(CoreString));
    if (coreString == NULL) {
        return NULL; // Memory allocation failed
    }
    coreString->length = strlen(str);
    coreString->str = (char *)malloc(coreString->length + 1);
    if (coreString->str == NULL) {
        free(coreString);
        return NULL; // Memory allocation failed
    }
    strcpy(coreString->str, str);
    return coreString;
}

CoreString * CoreString_create_with_length(const char *str, size_t length) {
    CoreString *coreString = (CoreString *)malloc(sizeof(CoreString));
    if (coreString == NULL) {
        return NULL; // Memory allocation failed
    }
    coreString->length = length;
    coreString->str = (char *)malloc(length + 1);
    if (coreString->str == NULL) {
        free(coreString);
        return NULL; // Memory allocation failed
    }
    if (length > 0) {
        strncpy(coreString->str, str, length);
    }
    coreString->str[length] = '\0'; // Null-terminate the string
    return coreString;
}

void CoreString_destroy(CoreString *coreString){
    if (coreString != NULL) {
        free(coreString->str);
        free(coreString);
        coreString = NULL; // Set to NULL to avoid dangling pointer
    }
}

void CoreString_append(CoreString *coreString, const char *str){
    size_t newLength = coreString->length + strlen(str);
    coreString->str = (char *)realloc(coreString->str, newLength + 1);
    if (coreString->str != NULL) {
        strcat(coreString->str, str);
        coreString->length = newLength;
    }
}

void CoreString_prepend(CoreString *coreString, const char *str){
    size_t newLength = coreString->length + strlen(str);
    coreString->str = (char *)realloc(coreString->str, newLength + 1);
    if (coreString->str != NULL) {
        memmove(coreString->str + strlen(str), coreString->str, coreString->length + 1);
        memcpy(coreString->str, str, strlen(str));
        coreString->length = newLength;
    }
}

void CoreString_insert(CoreString *coreString, size_t index, const char *str){
    if (index > coreString->length) {
        return;
    }

    size_t newLength = coreString->length + strlen(str);
    coreString->str = (char *)realloc(coreString->str, newLength + 1);
    if (coreString->str != NULL) {
        memmove(coreString->str + index + strlen(str), coreString->str + index, coreString->length - index + 1);
        memcpy(coreString->str + index, str, strlen(str));
        coreString->length = newLength;
    }
}

void CoreString_remove(CoreString *coreString, size_t index, size_t length){
    if (index >= coreString->length) {
        return;
    }

    if (index + length > coreString->length) {
        length = coreString->length - index;
    }
    memmove(coreString->str + index, coreString->str + index + length, coreString->length - index - length + 1);
    coreString->length -= length;
    coreString->str = (char *)realloc(coreString->str, coreString->length + 1);
}

void CoreString_replace(CoreString *coreString, size_t index, size_t length, const char *str){
    if (index >= coreString->length) {
        return;
    }

    if (index + length > coreString->length) {
        length = coreString->length - index;
    }

    size_t newLength = coreString->length - length + strlen(str);
    coreString->str = (char *)realloc(coreString->str, newLength + 1);
    if (coreString->str != NULL) {
        memmove(coreString->str + index + strlen(str), coreString->str + index + length, coreString->length - index - length + 1);
        memcpy(coreString->str + index, str, strlen(str));
        coreString->length = newLength;
    }
}

void CoreString_clear(CoreString *coreString){
    if (coreString != NULL) {
        free(coreString->str);
        coreString->str = NULL;
        coreString->length = 0;
    }
}
