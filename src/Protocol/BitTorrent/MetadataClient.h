#ifndef METADATACLIENT_H
#define METADATACLIENT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define CLIENT_PREFIX "-CT0010-"
#define PEER_ID_LEN 20  // Length of the peer ID string
#define INFO_HASH_LEN 20


typedef struct MetadataClient MetadataClient;

typedef enum {
    METADATA_OK             =  0,
    METADATA_ERROR_INIT     = -1,  // curl_global_init or client init failed
    METADATA_ERROR_CURL     = -2,  // underlying curl error
    METADATA_ERROR_MEM      = -3,  // allocation failure
    METADATA_ERROR_INVALID  = -4,  // bad arguments
    METADATA_ERROR_HTTP     = -5   // non-2xx HTTP response
} MetadataResult;

typedef struct {
    long   timeout_seconds;   // overall request timeout
    bool   follow_redirects;  // enable CURLOPT_FOLLOWLOCATION
    const char* user_agent;   // e.g. "cTorrent/0.1"
} MetadataOptions;

/// Used for tracker metadata fetching and announcing.
/// Requires an announce URL and a tracker URL.
MetadataClient* MetadataClient_create(const MetadataOptions* opts);
void MetadataClient_destroy(MetadataClient* client);

const char* MetadataClient_create_peer_id(void);

MetadataResult MetadataClient_announce(
    MetadataClient* client,
    const char*     url,
    const char*     info_hash,
    const char*     peer_id,
    uint64_t        listen_port,
    uint64_t        uploaded,
    uint64_t        downloaded,
    uint64_t        left,
    const char *event,
    uint32_t        num_want,
    uint32_t        compact,
    uint8_t**       out_buffer,
    size_t*         out_size
);

#endif //METADATACLIENT_H
