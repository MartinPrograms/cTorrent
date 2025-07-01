#include "CoreNetworking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t write_discard(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

CoreNetworkingBandwithTest *
CoreNetworking_bandwith_test_run(
    const CoreNetworkingBandwithTestOptions *options,
    size_t count,
    size_t max_concurrency)
{
    if (options == NULL || count == 0 || max_concurrency == 0) {
        return NULL;
    }

    CoreNetworkingBandwithTest *results =
        calloc(count, sizeof(CoreNetworkingBandwithTest));
    if (!results) {
        return NULL;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        free(results);
        return NULL;
    }

    CURLM *multi_handle = curl_multi_init();
    if (!multi_handle) {
        curl_global_cleanup();
        free(results);
        return NULL;
    }

    size_t next_to_add = 0;
    int still_running = 0;

    size_t initial = count < max_concurrency ? count : max_concurrency;
    for (; next_to_add < initial; ++next_to_add) {
        CURL *easy = curl_easy_init();
        if (!easy) continue;

        curl_easy_setopt(easy, CURLOPT_URL, options[next_to_add].url);
        char range_header[32];
        snprintf(range_header, sizeof(range_header),
                 "0-%zu", options[next_to_add].chunk_size - 1);
        curl_easy_setopt(easy, CURLOPT_RANGE, range_header);
        if (options[next_to_add].user_agent) {
            curl_easy_setopt(easy, CURLOPT_USERAGENT,
                             options[next_to_add].user_agent);
        }
        curl_easy_setopt(easy, CURLOPT_TIMEOUT,
                         options[next_to_add].timeout_seconds);
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION,
                         options[next_to_add].follow_redirects ? 1L : 0L);
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_discard);
        curl_easy_setopt(easy, CURLOPT_PRIVATE, (void *)next_to_add);

        curl_multi_add_handle(multi_handle, easy);
    }

    curl_multi_perform(multi_handle, &still_running);

    while (still_running) {
        int numfds = 0;
        curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
        curl_multi_perform(multi_handle, &still_running);

        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *easy = msg->easy_handle;

                size_t idx = 0;
                curl_easy_getinfo(easy, CURLINFO_PRIVATE, &idx);

                char *ct = NULL;
                curl_easy_getinfo(easy, CURLINFO_CONTENT_TYPE, &ct);

                if (ct && strcmp(ct, "text/html") == 0) {
                    results[idx].chunk_size = options[idx].chunk_size;
                    results[idx].time_to_connect = 0;
                    results[idx].time_to_first_byte = 0;
                    results[idx].total_time = 0;
                    results[idx].success = false;
                }
                else if (msg->data.result != CURLE_OK) {
                    results[idx].chunk_size = options[idx].chunk_size;
                    results[idx].time_to_connect = 0;
                    results[idx].time_to_first_byte = 0;
                    results[idx].total_time = 0;
                    results[idx].success = false;
                }
                else {
                    double t_connect = 0.0, t_firstbyte = 0.0, t_total = 0.0;
                    double size_download = 0;
                    curl_easy_getinfo(easy, CURLINFO_CONNECT_TIME,       &t_connect);
                    curl_easy_getinfo(easy, CURLINFO_STARTTRANSFER_TIME, &t_firstbyte);
                    curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME,         &t_total);
                    curl_easy_getinfo(easy, CURLINFO_SIZE_DOWNLOAD, &size_download);

                    // Cast size_download to size_t
                    if (size_download < 0) {
                        size_download = 0;
                    }
                    size_t chunk_size = (size_t)size_download;
                    if (chunk_size != options[idx].chunk_size) {
                        fprintf(stderr, "Warning: Downloaded size (%zu) does not match expected chunk size (%zu) for URL %s\n",
                                chunk_size, options[idx].chunk_size, options[idx].url);

                        results[idx].chunk_size = options[idx].chunk_size;
                        results[idx].time_to_connect = 0;
                        results[idx].time_to_first_byte = 0;
                        results[idx].total_time = 0;
                        results[idx].success = false;
                        continue;
                    }
                    results[idx].time_to_connect      = (size_t)(t_connect   * 1000);
                    results[idx].time_to_first_byte   = (size_t)(t_firstbyte * 1000);
                    results[idx].total_time           = (size_t)(t_total     * 1000);
                    results[idx].success              = true;
                }

                curl_multi_remove_handle(multi_handle, easy);
                curl_easy_cleanup(easy);

                if (next_to_add < count) {
                    CURL *next_easy = curl_easy_init();
                    if (next_easy) {
                        curl_easy_setopt(next_easy, CURLOPT_URL,
                                         options[next_to_add].url);
                        char range2[32];
                        snprintf(range2, sizeof(range2),
                                 "0-%zu",
                                 options[next_to_add].chunk_size - 1);
                        curl_easy_setopt(next_easy, CURLOPT_RANGE, range2);
                        if (options[next_to_add].user_agent) {
                            curl_easy_setopt(next_easy,
                                             CURLOPT_USERAGENT,
                                             options[next_to_add]
                                                 .user_agent);
                        }
                        curl_easy_setopt(next_easy,
                                         CURLOPT_TIMEOUT,
                                         options[next_to_add]
                                             .timeout_seconds);
                        curl_easy_setopt(next_easy,
                                         CURLOPT_FOLLOWLOCATION,
                                         options[next_to_add]
                                             .follow_redirects ? 1L : 0L);
                        curl_easy_setopt(next_easy,
                                         CURLOPT_WRITEFUNCTION,
                                         write_discard);
                        curl_easy_setopt(next_easy,
                                         CURLOPT_PRIVATE,
                                         (void *)next_to_add);

                        curl_multi_add_handle(multi_handle, next_easy);
                    }
                    ++next_to_add;
                    curl_multi_perform(multi_handle, &still_running);
                }
            }
        }
    }

    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();

    return results;
}

static int progress_callback(void *clientp,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t ultotal,
                             curl_off_t ulnow) {
    if (dltotal == 0) return 0;

    int width = 50; // Progress bar width
    int percent = (int)((dlnow * 100) / dltotal);
    int filled = (int)((dlnow * width) / dltotal);

    fprintf(stderr, "\r[");
    for (int i = 0; i < width; ++i) {
        fputc(i < filled ? '=' : ' ', stderr);
    }
    fprintf(stderr, "] %d%%", percent);
    fflush(stderr);

    return 0; // Returning non-zero aborts the transfer
}

bool CoreNetworking_download_to_file(const CoreNetworkingDownloadOptions *options, CoreFile* download_file) {
    if (options == NULL || download_file == NULL) {
        fprintf(stderr, "CoreNetworking_download_to_buffer: Invalid arguments\n");
        return false;
    }

    FILE* file = download_file->file;
    if (!file) {
        fprintf(stderr, "CoreNetworking_download_to_buffer: File not open\n");
        return false;
    }

    CURL *easy = curl_easy_init();
    if (!easy) {
        fprintf(stderr, "CoreNetworking_download_to_buffer: curl_easy_init failed\n");
        return false;
    }

    curl_easy_setopt(easy, CURLOPT_URL, options->url);
    if (options->user_agent) {
        curl_easy_setopt(easy, CURLOPT_USERAGENT, options->user_agent);
    }

    // Disable the timeout
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, options->follow_redirects ? 1L : 0L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L); // Fail on HTTP errors

    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, NULL);

    CURLcode res = curl_easy_perform(easy);
    if (res != CURLE_OK) {
        fprintf(stderr, "CoreNetworking_download_to_buffer: curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(easy);
        return false;
    }
    curl_easy_cleanup(easy);
    return true;
}
