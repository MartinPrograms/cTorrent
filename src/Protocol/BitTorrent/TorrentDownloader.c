// TorrentDownloader.c
#include "TorrentDownloader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CommonCrypto/CommonDigest.h>
#include <CoreNetworking.h> // for test_network
#include <MetadataClient.h>

// Helper to lookup dictionary entries
static BencodeItem* get_dict_value(BencodeDictionary* dict, const char* key) {
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(dict->keys[i]->str, key) == 0)
            return dict->values[i];
    }
    return NULL;
}

// Compute SHA1 hex string of a file at 'path'
static char* compute_sha1(const char* path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    CC_SHA1_CTX ctx;
    CC_SHA1_Init(&ctx);

    unsigned char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        CC_SHA1_Update(&ctx, buf, (CC_LONG)n);
    }
    fclose(f);

    unsigned char hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1_Final(hash, &ctx);

    char* hex = malloc(2 * CC_SHA1_DIGEST_LENGTH + 1);
    if (!hex) return NULL;
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++) {
        sprintf(hex + i*2, "%02x", hash[i]);
    }
    hex[2*CC_SHA1_DIGEST_LENGTH] = '\0';
    return hex;
}

TorrentDownloader* TorrentDownloader_create(BencodeItem* torrent_item,
                                            const char* output_path) {
    if (!torrent_item || torrent_item->type != BENCODE_TYPE_DICTIONARY)
        return NULL;

    TorrentDownloader *dl = calloc(1, sizeof(*dl));
    dl->output_path = strdup(output_path);
    dl->info.meta    = torrent_item->value.dictionary;

    // Inspect “info” section
    BencodeItem *info = get_dict_value(dl->info.meta, "info");
    if (info && info->type == BENCODE_TYPE_DICTIONARY) {
        BencodeDictionary *infod = info->value.dictionary;
        if (get_dict_value(infod, "files")) {
            dl->info.type = TORRENT_MULTI_FILE;  // unsupported
        } else {
            dl->info.type      = TORRENT_SINGLE_FILE;
            dl->info.file_count = 1;
            dl->info.files      = calloc(1, sizeof(TorrentFileInfo));
            BencodeItem *name   = get_dict_value(infod, "name");
            BencodeItem *length = get_dict_value(infod, "length");
            dl->info.files[0].file_name = name->value.string->str;
            dl->info.files[0].file_size = length->value.integer;
            // Get the sha1 hash from the piece
        }
    } else {
        dl->info.type = TORRENT_UNKNOWN;
    }

    // DDL vs tracker
    if (get_dict_value(dl->info.meta, "url-list")) {
        dl->is_ddl = true;
    } else {
        dl->is_ddl = false;
        BencodeItem* ann = get_dict_value(dl->info.meta, "announce");
        dl->url = ann ? ann->value.string->str : NULL;
    }

    return dl;
}

void TorrentDownloader_destroy(TorrentDownloader* dl) {
    if (!dl) return;
    free(dl->output_path);
    free(dl->info.files);
    free(dl);
}

void TorrentDownloader_print_info(const TorrentDownloader* dl) {
    printf("Torrent type: %s\n",
        dl->info.type == TORRENT_SINGLE_FILE ? "Single-file" :
        dl->info.type == TORRENT_MULTI_FILE  ? "Multi-file"  :
                                               "Unknown");
    for (int i = 0; i < dl->info.file_count; i++) {
        const TorrentFileInfo *f = &dl->info.files[i];
        printf("File: %s (%llu bytes)\n",
               f->file_name, (unsigned long long)f->file_size);
    }
    printf("Download method: %s\n",
           dl->is_ddl ? "DDL" : "Tracker");
    if (dl->url) printf("URL: %s\n", dl->url);
}

const char* test_network(BencodeItem *url_list, const char* file_name, uint64_t file_size) {
    if (!url_list || url_list->type != BENCODE_TYPE_LIST) {
        printf("Invalid 'url-list'\n");
        return NULL;
    }

    size_t count = url_list->value.list->count;
    printf("Found %zu URLs\n", count);
    if (!count) return NULL;
    if (count > MAX_TEST_NETWORK) {
        printf("Limiting to %d URLs\n", MAX_TEST_NETWORK);
        count = MAX_TEST_NETWORK;
    }

    CoreNetworkingBandwithTestOptions* options = calloc(count, sizeof(CoreNetworkingBandwithTestOptions));
    if (!options) {
        fprintf(stderr, "Allocation failed\n");
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        BencodeItem *url_item = &url_list->value.list->items[i];
        if (url_item->type != BENCODE_TYPE_STRING) {
            printf("Skipping invalid URL at %zu\n", i);
            continue;
        }

        CoreString_append(url_item->value.string, file_name);
        options[i] = (CoreNetworkingBandwithTestOptions){
            .url = url_item->value.string->str,
            .timeout_seconds = 5,
            .follow_redirects = true,
            .chunk_size = (file_size < 32768) ? file_size : 32768,
            .user_agent = "cTorrent/0.1"
        };
    }

    printf("Testing %zu URLs...\n", count);
    CoreNetworkingBandwithTest *results = CoreNetworking_bandwith_test_run(options, count, MAX_PARALLEL_NETWORK);
    if (!results) {
        fprintf(stderr, "Bandwidth test failed\n");
        free(options);
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        printf("\nURL: %s\n", options[i].url);
        if (results[i].success) {
            printf("Chunk: %zu bytes\nConnect: %zu ms\nFirst Byte: %zu ms\nTotal: %zu ms\n",
                   results[i].chunk_size, results[i].time_to_connect,
                   results[i].time_to_first_byte, results[i].total_time);
        } else {
            printf("Test failed\n");
        }
    }

    // Find the fastest 5 URLs
    size_t fastest_count = count < 5 ? count : 5;
    char* fastest_urls[fastest_count];
    printf("\nFastest URLs:\n");
    for (size_t i = 0; i < fastest_count; i++) {
        size_t min_index = 0;
        for (size_t j = 1; j < count; j++) {
            if (results[j].success && results[j].total_time < results[min_index].total_time) {
                min_index = j;
            }
        }
        if (results[min_index].success) {
            printf("%s - %zu ms\n", options[min_index].url, results[min_index].total_time);
            fastest_urls[i] = (char*)options[min_index].url;
        } else {
            printf("No more successful tests\n");
            break;
        }
        results[min_index].success = false; // Mark as used
    }
    printf("\nFastest URLs:\n");
    for (size_t i = 0; i < fastest_count; i++) {
        if (fastest_urls[i]) {
            printf("%s\n", fastest_urls[i]);
        }
    }

    free(results);
    free(options);

    return fastest_urls[0]; // Return the first fastest URL
}

bool download_as_ddl(TorrentDownloader *dl) {
    BencodeItem* url_list =
            get_dict_value(dl->info.meta, "url-list");
    const char* fastest =
            test_network(url_list,
                         dl->info.files[0].file_name,
                         dl->info.files[0].file_size);
    if (!fastest) {
        printf("No valid URL found\n");
        return true;
    }
    printf("Using fastest URL: %s\n", fastest);

    // Build output path
    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/%s",
             dl->output_path, dl->info.files[0].file_name);

    CoreFile *out = CoreFile_create(outpath, "wb+");
    if (!out) {
        fprintf(stderr, "Cannot create %s\n", outpath);
        return true;
    }

    CoreNetworkingDownloadOptions opts = {
        .url              = fastest,
        .timeout_seconds  = 30,
        .follow_redirects = true,
        .user_agent       = "cTorrent/0.1"
    };

    if (!CoreNetworking_download_to_file(&opts, out)) {
        fprintf(stderr, "Download failed\n");
        CoreFile_close(out);
        return true;
    }
    CoreFile_flush(out);
    CoreFile_close(out);
    return false;
}

bool download_as_tracker(TorrentDownloader * dl) {
    // Get the info hash and the url
    BencodeItem *info = get_dict_value(dl->info.meta, "info");
    if (!info || info->type != BENCODE_TYPE_DICTIONARY) {
        fprintf(stderr, "Invalid torrent metadata\n");
        return false;
    }
    BencodeDictionary *infod = info->value.dictionary;
    BencodeItem *name = get_dict_value(infod, "name");
    if (!name || name->type != BENCODE_TYPE_STRING) {
        fprintf(stderr, "Torrent name not found\n");
        return false;
    }

    uint8_t* info_hash = BencodeItem_compute_sha1(info);

    // Q: How do we get the info hash?
    char hex[CC_SHA1_DIGEST_LENGTH*2 + 1];
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++)
        sprintf(hex + i*2, "%02x", info_hash[i]);
    hex[CC_SHA1_DIGEST_LENGTH*2] = '\0';
    printf("Info hash: %s\n", hex);
    if (dl->url == NULL) {
        fprintf(stderr, "No tracker URL found\n");
        free(info_hash);
        return false;
    }
    printf("Tracker URL: %s\n", dl->url);

    // We should assume this is a multifile torrent, so the length property is done per file.

    // Now we can request the tracker
    MetadataOptions opts = {
        .user_agent = "cTorrent/0.1",
        .timeout_seconds = 30,
        .follow_redirects = true
    };
    MetadataClient* client = MetadataClient_create(&opts);
    if (!client) {
        fprintf(stderr, "Failed to create MetadataClient\n");
        free(info_hash);
        return false;
    }

    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    MetadataClient_announce(client, dl->url, hex, MetadataClient_create_peer_id(), 6969, 0, 0, 0, "started",
                            50, 1, &buffer, &buffer_size);

    printf("Tracker response size: %zu bytes\n", buffer_size);

    return true;
}

void TorrentDownloader_download(TorrentDownloader* dl) {
    if (!dl->is_ddl) {
        if (download_as_tracker(dl)) {printf("Successfully downloaded as tracker\n");}
        else {printf("Failed to download as tracker\n");}
        return;
    }

    if (download_as_ddl(dl)) return;

    // TODO: Implement a hash check (idk how to do this yet)
}
