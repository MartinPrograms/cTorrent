#ifndef TORRENTDOWNLOADER_H
#define TORRENTDOWNLOADER_H

// This downloads torrent files to disk!

#include <stdint.h>
#include <CoreFile.h>
#include <Bencode.h>
#include <CoreNetworking.h>

#define MAX_TEST_NETWORK 500
#define MAX_PARALLEL_NETWORK 100

typedef enum {
    TORRENT_SINGLE_FILE,
    TORRENT_MULTI_FILE,
    TORRENT_UNKNOWN
} TorrentType;

typedef struct {
    const char* file_name;
    uint64_t file_size; // in bytes
    char* hash; // SHA1 hash of the file
} TorrentFileInfo;

typedef struct {
    const char *path;
    BencodeDictionary *meta;
    TorrentFileInfo *files; // array of files in the torrent
    int file_count; // number of files in the torrent
    TorrentType type;
} TorrentInfo;

typedef struct {
    TorrentInfo info;
    char* output_path;
    bool is_ddl; // true if this is a DDL torrent (no tracker)
    char* url; // URL to download from if it's a DDL torrent, otherwise its the announce URL
} TorrentDownloader;

// Initialise the torrent downloader with a bencode item!
TorrentDownloader *TorrentDownloader_create(BencodeItem *torrent_item, const char* output_path);
void TorrentDownloader_destroy(TorrentDownloader *downloader);

void TorrentDownloader_print_info(const TorrentDownloader *downloader);
void TorrentDownloader_download(TorrentDownloader *downloader);

#endif //TORRENTDOWNLOADER_H
