#include "CoreFile.h"

char* getFileName(const char* path) {
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL) {
        return strdup(lastSlash + 1);
    }
    return strdup(path);
}

CoreFile *CoreFile_open(const char *path, const char *mode) {
    if (path == NULL || mode == NULL) return NULL;

    CoreFile *file = malloc(sizeof(CoreFile));
    if (!file) return NULL;

    file->file = fopen(path, mode);
    if (!file->file) {
        free(file);
        return NULL;
    }

    file->name = getFileName(path);
    file->path = strdup(path);

    fseek(file->file, 0, SEEK_END);
    file->size = ftell(file->file);
    fseek(file->file, 0, SEEK_SET);

    return file;
}

CoreFile * CoreFile_create(const char *path, const char *mode) {
    return CoreFile_open(path, mode);
}

uint64_t CoreFile_get_size(CoreFile *file) {
    if (!file || !file->file) return 0;
    long curr = ftell(file->file);
    fseek(file->file, 0, SEEK_END);
    uint64_t size = ftell(file->file);
    fseek(file->file, curr, SEEK_SET);
    return size;
}

bool CoreFile_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

void CoreFile_close(CoreFile *file) {
    if (!file) return;
    if (file->file) fclose(file->file);
    free(file->name);
    free(file->path);
    free(file);
}

void CoreFile_delete(CoreFile *file) {
    if (!file) return;
    if (file->file) fclose(file->file);
    if (file->path) remove(file->path); // Difference between delete and close
    free(file->name);
    free(file->path);
    free(file);
}

unsigned long CoreFile_read(CoreFile *file, void *buffer, size_t size) {
    if (!file || !file->file || !buffer) return -1;
    return fread(buffer, 1, size, file->file);
}

unsigned long CoreFile_write(CoreFile *file, const void *data, size_t size) {
    if (!file || !file->file || !data) return -1;
    return fwrite(data, 1, size, file->file);
}

void CoreFile_read_chunk(CoreFile *file, void *buffer, size_t size, uint64_t offset) {
    if (!file || !file->file || !buffer) return;
    fseek(file->file, (long)offset, SEEK_SET);
    fread(buffer, 1, size, file->file);
}

void CoreFile_write_chunk(CoreFile *file, const void *data, size_t size, uint64_t offset) {
    if (!file || !file->file || !data) return;
    fseek(file->file, (long)offset, SEEK_SET);
    fwrite(data, 1, size, file->file);
}

void CoreFile_preallocate(CoreFile *file, uint64_t size) {
    if (!file || !file->file) return;
    fseek(file->file, (long)size - 1, SEEK_SET);
    fputc('\0', file->file);
    fflush(file->file);
    file->size = size;
}

void CoreFile_flush(CoreFile *file) {
    if (!file || !file->file) return;
    fflush(file->file);
}
