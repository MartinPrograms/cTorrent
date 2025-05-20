#ifndef COREFILE_H
#define COREFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// We use a custom abstraction on top of the standard C file I/O functions
// This allows for easier piece management later.

typedef struct{
    char *name;
    char *path;
    uint64_t size;
    FILE *file;
} CoreFile;

CoreFile *CoreFile_open(const char *path, const char *mode);

#endif //COREFILE_H
