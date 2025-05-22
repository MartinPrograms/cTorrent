#ifndef COREFILE_H
#define COREFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// We use a custom abstraction on top of the standard C file I/O functions
// This allows for easier piece management later.

typedef struct{
    char *name;
    char *path;
    uint64_t size;
    FILE *file;
} CoreFile;

#define COREFILE_SUCCESS 0

CoreFile *CoreFile_open(const char *path, const char *mode);
CoreFile *CoreFile_create(const char *path, const char *mode);

uint64_t CoreFile_get_size(CoreFile *file);
bool CoreFile_exists(const char* path);

void CoreFile_close(CoreFile *file);
void CoreFile_delete(CoreFile *file);

unsigned long CoreFile_read(CoreFile *file, void *buffer, size_t size);
unsigned long CoreFile_write(CoreFile *file, const void *data, size_t size);
void CoreFile_write_chunk(CoreFile *file, const void *data, size_t size, uint64_t offset);
void CoreFile_read_chunk(CoreFile *file, void *buffer, size_t size, uint64_t offset);

void CoreFile_preallocate(CoreFile *file, uint64_t size);
void CoreFile_flush(CoreFile *file);

#endif //COREFILE_H
