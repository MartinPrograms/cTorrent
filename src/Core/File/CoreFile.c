#include "CoreFile.h"

char* getFileName(const char* path) {
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL) {
        return strdup(lastSlash + 1);
    }
    return strdup(path);
}

CoreFile * CoreFile_open(const char *path, const char *mode) {
    CoreFile *file = malloc(sizeof(CoreFile));
    if (file == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    if (path == NULL || mode == NULL) {
        fprintf(stderr, "Error: Path or mode is NULL\n");
        free(file);
        return NULL;
    }

    // Open the file
    file->file = fopen(path, mode);
    if (file->file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", path);
        free(file);
        return NULL;
    }

    file->name = getFileName(path);
    if (file->name == NULL) {
        fprintf(stderr, "Error: Could not get file name from path %s\n", path);
        fclose(file->file);
        free(file);
        return NULL;
    }

    file->size = 0;
    fseek(file->file, 0, SEEK_END);
    file->size = ftell(file->file);
    fseek(file->file, 0, SEEK_SET);
    if (file->size == (uint64_t) -1) {
        fprintf(stderr, "Error: Could not get file size for %s\n", path);
        fclose(file->file);
        free(file->name);
        free(file);
        return NULL;
    }

    file->path = strdup(path);
    if (file->path == NULL) {
        fprintf(stderr, "Error: Could not duplicate path %s\n", path);
        fclose(file->file);
        free(file->name);
        free(file);
        return NULL;
    }

    // Done
    return file;
}
