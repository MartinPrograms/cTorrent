#ifndef CORENETWORKING_H
#define CORENETWORKING_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <CoreFile.h>

typedef struct {
  size_t chunk_size;
  size_t time_to_first_byte;
  size_t total_time;
  size_t time_to_connect;
  bool success; // true if the test was successful, false if there was an error
} CoreNetworkingBandwithTest;

typedef struct {
  const char *url;
  const char *user_agent; // e.g. "cTorrent/0.1"
  size_t timeout_seconds; // overall request timeout
  bool follow_redirects;  // enable CURLOPT_FOLLOWLOCATION
  size_t chunk_size;      // size of each chunk to download in bytes
} CoreNetworkingBandwithTestOptions;

typedef struct {
  const char* url;
  const char* user_agent;
  size_t timeout_seconds;
  bool follow_redirects;
} CoreNetworkingDownloadOptions;

/// Runs a bandwidth test against the given URLs using the provided options.
/// Options should be an array of CoreNetworkingBandwithTestOptions.
/// All results are in order of the URLs provided.
CoreNetworkingBandwithTest *CoreNetworking_bandwith_test_run(const CoreNetworkingBandwithTestOptions *options, size_t count, size_t max_concurrent);
bool CoreNetworking_download_to_file(
    const CoreNetworkingDownloadOptions *options,
    CoreFile *file
);
#endif //CORENETWORKING_H
