// main.c
#include <stdio.h>
#include <stdlib.h>
#include <CoreFile.h>
#include <Bencode.h>
#include "TorrentDownloader.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

int main(void) {
    const char* path = "/Users/martin/Downloads/torrent2.torrent";
    CoreFile* f = CoreFile_open(path, "rb");
    if (!f) {
        fprintf(stderr, "Can't open %s\n", path);
        return 1;
    }

    uint64_t sz = CoreFile_get_size(f);
    char *buf = malloc(sz + 1);
    if (!buf || CoreFile_read(f, buf, sz) != sz) {
        fprintf(stderr, "Read error\n");
        CoreFile_close(f);
        return 1;
    }
    buf[sz] = '\0';
    CoreFile_close(f);

    BencodeItem *root = BencodeItem_parse(buf, sz);
    free(buf);
    if (!root || root->type != BENCODE_TYPE_DICTIONARY) {
        fprintf(stderr, "Invalid torrent\n");
        return 1;
    }

    DIR* dir = opendir("./downloads");
    if (dir) {
        closedir(dir);
    } else {
        if (mkdir("./downloads", 0755) != 0) {
            fprintf(stderr, "Failed to create downloads directory\n");
            return 1;
        }
    }

    // Create and use TorrentDownloader
    TorrentDownloader *dl = TorrentDownloader_create(root, "./downloads");
    if (!dl) {
        fprintf(stderr, "Failed to initialize downloader\n");
        return 1;
    }

    TorrentDownloader_print_info(dl);
    TorrentDownloader_download(dl);
    TorrentDownloader_destroy(dl);

    return 0;
}
