#include "MetadataClient.h"

#include <string.h>
#include <curl/curl.h>
struct MetadataClient {
    CURL*          curl_handle;
    MetadataOptions opts;
};

MetadataClient * MetadataClient_create(const MetadataOptions *opts) {
    // The metadata client uses libcurl to fetch metadata from a tracker.
    MetadataClient *client = (MetadataClient *)malloc(sizeof(MetadataClient));
    if (!client) {
        return NULL; // Memory allocation failed
    }

    client->curl_handle = curl_easy_init();
    if (!client->curl_handle) {
        free(client);
        return NULL; // libcurl initialization failed
    }

    client->opts.user_agent = opts->user_agent ? opts->user_agent : "cTorrent/0.1"; // Default user agent
    client->opts.timeout_seconds = opts->timeout_seconds > 0 ? opts->timeout_seconds : 30; // Default to 30 seconds
    client->opts.follow_redirects = opts->follow_redirects;

    curl_easy_setopt(client->curl_handle, CURLOPT_TIMEOUT,
                 client->opts.timeout_seconds);
    curl_easy_setopt(client->curl_handle, CURLOPT_FOLLOWLOCATION,
                     client->opts.follow_redirects);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT,
                     client->opts.user_agent);

    return client;
}

void MetadataClient_destroy(MetadataClient *client) {
    if (client == NULL) {
        return; // Nothing to destroy
    }

    // Destroy the curl context
    if (client->curl_handle) {
        curl_easy_cleanup(client->curl_handle);
    }
    free(client); // Free the client structure
    client = NULL;
}

const char* MetadataClient_create_peer_id(void) {
    char* out = malloc(21); // 20 characters + null terminator
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t suffix_len = PEER_ID_LEN - strlen(CLIENT_PREFIX);  // 12
    size_t i;

    strcpy(out, CLIENT_PREFIX);
    srand((unsigned int)time(NULL) ^ (uintptr_t)&out);  // basic randomness

    for (i = strlen(CLIENT_PREFIX); i < PEER_ID_LEN; ++i) {
        int key = rand() % (int)(sizeof(charset) - 1);
        out[i] = charset[key];
    }
    out[PEER_ID_LEN] = '\0'; // Null-terminate
    return out; // Return the generated peer ID
}

int hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    if (strlen(hex) != out_len * 2) return -1;
    for (size_t i = 0; i < out_len; ++i) {
        if (sscanf(hex + 2 * i, "%2hhx", &out[i]) != 1)
            return -1;
    }
    return 0;
}

static size_t write_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct MemoryBuffer {
        uint8_t *data;
        size_t size;
    } *mem = userdata;

    uint8_t *new_data = realloc(mem->data, mem->size + total);
    if (!new_data) return 0;

    memcpy(new_data + mem->size, ptr, total);
    mem->data = new_data;
    mem->size += total;

    return total;
}

void url_encode(const uint8_t *src, size_t len, char *dest) {
    for (size_t i = 0; i < len; i++) {
        sprintf(dest + 3*i, "%%%02X", src[i]);
    }
}

MetadataResult MetadataClient_announce(
    MetadataClient* client,
    const char*     url,
    const char*     info_hash_hex,
    const char*     peer_id,
    uint64_t        listen_port,
    uint64_t        uploaded,
    uint64_t        downloaded,
    uint64_t        left,
    const char *    event,
    uint32_t        num_want,
    uint32_t        compact,
    uint8_t**       out_buffer,
    size_t*         out_size)
{
    if (!info_hash_hex || strlen(info_hash_hex) != 40) return METADATA_ERROR_INVALID;
    if (!out_buffer || !out_size) return METADATA_ERROR_INVALID;


    // Convert info_hash_hex to raw bytes
    uint8_t info_hash_raw[INFO_HASH_LEN];
    if (hex_to_bytes(info_hash_hex, info_hash_raw, INFO_HASH_LEN) != 0)
        return METADATA_ERROR_INVALID;

    // URL-encode info_hash and peer_id
    char encoded_info_hash[INFO_HASH_LEN * 2 + 1];
    url_encode(info_hash_raw, INFO_HASH_LEN, encoded_info_hash);
    // Use standard BitTorrent port range
    listen_port = (listen_port >= 6881 && listen_port <= 6889)
                  ? listen_port : 6881;
    // Build full URL with query parameters
    char full_url[2048];
    snprintf(full_url, sizeof(full_url),
             "%s?info_hash=%s&peer_id=%s&port=%llu&uploaded=%llu&downloaded=%llu&left=%llu&event=%s&numwant=%u&compact=%u",
             url,
             encoded_info_hash,
             peer_id,
             listen_port,
             uploaded,
             downloaded,
             left,
             event ? event : "started",
             num_want,
             compact);

    // Prepare memory buffer for response
    struct MemoryBuffer {
        uint8_t *data;
        size_t size;
    } mem = {0};

    // Set CURL options
    curl_easy_setopt(client->curl_handle, CURLOPT_URL, full_url);
    curl_easy_setopt(client->curl_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(client->curl_handle, CURLOPT_TIMEOUT, client->opts.timeout_seconds);
    curl_easy_setopt(client->curl_handle, CURLOPT_FOLLOWLOCATION, client->opts.follow_redirects);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT, client->opts.user_agent);
    curl_easy_setopt(client->curl_handle, CURLOPT_CONNECTTIMEOUT, client->opts.timeout_seconds);
    curl_easy_setopt(client->curl_handle, CURLOPT_TIMEOUT_MS, client->opts.timeout_seconds * 1000L);
    curl_easy_setopt(client->curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(client->curl_handle, CURLOPT_SSL_VERIFYPEER, 0L); // Only for dev

    // Set up write callback
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEFUNCTION, write_to_memory);
    curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, &mem);

    CURLcode res = curl_easy_perform(client->curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        if (mem.data) free(mem.data);
        return METADATA_ERROR_CURL;
    }

    long http_code = 0;
    curl_easy_getinfo(client->curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        if (mem.data) free(mem.data);
        return METADATA_ERROR_HTTP;
    }

    *out_buffer = mem.data;
    *out_size = mem.size;

    fprintf(stderr, "MetadataClient_announce: Successfully announced to %s\n", url);
    fprintf(stderr, "with url: %s\n", full_url);
    fprintf(stderr, "Response size: %zu bytes\n", *out_size);
    if (*out_size > 0) {
        fprintf(stderr, "Response data: ");
        for (size_t i = 0; i < *out_size && i < 64; ++i) {
            fprintf(stderr, "%02x ", (*out_buffer)[i]);
        }
        fprintf(stderr, "\n");
    }
    // After receiving response
    if (mem.size < 4 || mem.data[0] != 'd') {
        // Not a valid bencoded dictionary
        free(mem.data);
        return METADATA_ERROR_INVALID;
    }
    return METADATA_OK;
}
