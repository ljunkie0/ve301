/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include "spotify.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "player_if.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct struct_spotify_data {
    char host[256];
    struct lws_context *context;
    struct lws *web_socket;
    int connected;
    int show_cover;
} __spotify_data;

player *__spotify_player;
static char *__spotify_cover_url = NULL;
static char *__spotify_cover_path = NULL;

static int __spotify_connect();
static int __spotify_download_cover(const char *url);
static int __spotify_file_exists(const char *path);
static const char *__spotify_cover_ext_for_type(const char *content_type);

static void __spotify_fill_info(cJSON *metadata) {
    // Extract metadata information
    cJSON *uri = cJSON_GetObjectItem(metadata, "uri");
    cJSON *name = cJSON_GetObjectItem(metadata, "name");
    cJSON *artist_names = cJSON_GetObjectItem(metadata, "artist_names");
    cJSON *album_name = cJSON_GetObjectItem(metadata, "album_name");
    cJSON *album_cover_url = cJSON_GetObjectItem(metadata, "album_cover_url");
    cJSON *position = cJSON_GetObjectItem(metadata, "position");
    cJSON *duration = cJSON_GetObjectItem(metadata, "duration");

    player_set_title(__spotify_player, name ? name->valuestring : NULL);
    player_set_album(__spotify_player,
                     album_name ? album_name->valuestring : NULL);
    player_set_artist(__spotify_player,
                      artist_names ? artist_names->valuestring : NULL);
    if (__spotify_data.show_cover
        && player_set_cover_uri(__spotify_player,
                                album_cover_url ? album_cover_url->valuestring : NULL)) {
        if (album_cover_url && album_cover_url->valuestring) {
            __spotify_download_cover(album_cover_url->valuestring);
        } else {
            player_set_cover_image_path(__spotify_player, NULL);
        }
    } else {
        player_set_cover_image_path(__spotify_player, NULL);
    }

    // Print out the track information
    if (uri)
        log_config(SPOTIFY_CTX, "Track URI: %s\n", uri->valuestring);
    if (name)
        log_config(SPOTIFY_CTX, "Track Name: %s\n", name->valuestring);
    if (artist_names)
        log_config(SPOTIFY_CTX, "Artists: %s\n", artist_names->valuestring);
    if (album_name)
        log_config(SPOTIFY_CTX, "Album: %s\n", album_name->valuestring);
    if (album_cover_url)
        log_config(SPOTIFY_CTX, "Album Cover: %s\n", album_cover_url->valuestring);
    if (position)
        log_config(SPOTIFY_CTX, "Position: %d ms\n", position->valueint);
    if (duration)
        log_config(SPOTIFY_CTX, "Duration: %d ms\n", duration->valueint);
}

static int __spotify_file_exists(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && st.st_size > 0;
}

static const char *__spotify_cover_ext_for_type(const char *content_type) {
    if (!content_type) {
        return ".jpg";
    }
    if (strstr(content_type, "png")) {
        return ".png";
    }
    if (strstr(content_type, "jpeg") || strstr(content_type, "jpg")) {
        return ".jpg";
    }
    return ".jpg";
}

static int __spotify_download_cover(const char *url) {
    if (!url || !__spotify_player) {
        return 0;
    }
    if (__spotify_cover_url
        && strcmp(__spotify_cover_url, url) == 0
        && __spotify_cover_path
        && __spotify_file_exists(__spotify_cover_path)) {
        player_set_cover_image_path(__spotify_player, __spotify_cover_path);
        return 1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return 0;
    }

    FILE *fp = fopen("/tmp/ve301_spotify_cover.tmp", "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        log_warning(SPOTIFY_CTX, "Failed to download cover: %s\n",
                    curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        remove("/tmp/ve301_spotify_cover.tmp");
        player_set_cover_image_path(__spotify_player, NULL);
        return 0;
    }

    const char *content_type = NULL;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    const char *ext = __spotify_cover_ext_for_type(content_type);

    char path[256];
    snprintf(path, sizeof(path), "/tmp/ve301_spotify_cover%s", ext);
    rename("/tmp/ve301_spotify_cover.tmp", path);

    free_and_set_null((void **) &__spotify_cover_url);
    __spotify_cover_url = my_copystr(url);

    free_and_set_null((void **) &__spotify_cover_path);
    __spotify_cover_path = my_copystr(path);
    player_set_cover_image_path(__spotify_player, __spotify_cover_path);

    curl_easy_cleanup(curl);
    return 1;
}

static int __spotify_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {

    if (reason == LWS_CALLBACK_CLIENT_ESTABLISHED) {
        __spotify_data.connected = 1;
        log_info(SPOTIFY_CTX, "WebSocket connected\n");
    } else if (reason == LWS_CALLBACK_CLIENT_RECEIVE) {
        if (!in || len == 0) {
            return 0;
        }

        char *message = malloc(len + 1);
        if (!message) {
            log_error(SPOTIFY_CTX, "Out of memory while parsing websocket message\n");
            return 0;
        }
        memcpy(message, in, len);
        message[len] = '\0';

        log_config(SPOTIFY_CTX, "Received message: %s\n", message);
        cJSON *event = cJSON_Parse(message);
        if (event == NULL) {
            log_error(SPOTIFY_CTX, "Error parsing JSON\n");
            free(message);
            return 0;
        }
        log_config(SPOTIFY_CTX, "Event: %p\n", event);
        cJSON *type = cJSON_GetObjectItem(event, "type");
        if (!cJSON_IsString(type) || !type->valuestring) {
            log_error(SPOTIFY_CTX, "Event missing type\n");
            cJSON_Delete(event);
            free(message);
            return 0;
        }
        log_config(SPOTIFY_CTX, "Type: %s\n", type->valuestring);

        // Process the events based on their content
        if (strstr(type->valuestring, "inactive")) {
            log_config(SPOTIFY_CTX, "Device is now inactive.\n");
            player_set_active(__spotify_player, 0);
            player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
        } else {
            // Any other event implies an active device
            if (strstr(type->valuestring, "active")) {
                log_config(SPOTIFY_CTX, "Device is now active.\n");
                player_set_active(__spotify_player, 1);
            } else if (strstr(type->valuestring, "playing")
                       || strstr(type->valuestring, "will_play")) {
                player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_PLAYING);
            } else if (strstr(type->valuestring, "paused")) {
                player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_PAUSED);
            } else if (strstr(type->valuestring, "stopped")
                       || strstr(type->valuestring, "not_playing")) {
                player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
            } else if (strstr(type->valuestring, "metadata")) {
                log_config(SPOTIFY_CTX, "Metadata changed.\n");
                cJSON *metadata = cJSON_GetObjectItem(event, "data");
                if (metadata != NULL) {
                    __spotify_fill_info(metadata);
                }
            }
        }

        cJSON_Delete(event);
        free(message);
    } else if (reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR) {
        log_config(SPOTIFY_CTX, "Connection error: %s\n", (char *) in);
        __spotify_data.connected = 0;
        __spotify_data.web_socket = NULL;
        player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
        player_set_active(__spotify_player, 0);
    } else if (reason == LWS_CALLBACK_CLIENT_WRITEABLE) {
        // lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
        // return -1;
    } else if (reason == LWS_CALLBACK_CLIENT_CLOSED) {
        log_info(SPOTIFY_CTX, "WebSocket connection closed\n");
        __spotify_data.connected = 0;
        __spotify_data.web_socket = NULL;
        player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
        player_set_active(__spotify_player, 0);
    }
    return 0;
}

enum protocols { PROTOCOL_EXAMPLE = 0, PROTOCOL_COUNT };

static struct lws_protocols protocols[] = {
    {
        .name = "ws",                   /* Protocol name*/
        .callback = __spotify_callback, /* Protocol callback */
        .per_session_data_size = 0,     /* Protocol callback 'userdata' size */
        .rx_buffer_size = 0,            /* Receve buffer size (0 = no restriction) */
        .id = 0,                        /* Protocol Id (version) (optional) */
        .user = NULL,                   /* 'User data' ptr, to access in 'protocol callback */
        .tx_packet_size = 0 /* Transmission buffer size restriction (0 = no restriction) */
    },
    {NULL, NULL, 0, 0, 0, NULL, 0} /* terminator */
};

struct __spotify_memory_struct {
    char *memory;
    size_t size;
};

// Callback function for writing received data into memory
static size_t __spotify_write_memory_callback(void *contents, size_t size,
                                              size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct __spotify_memory_struct *mem = (struct __spotify_memory_struct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        log_error(SPOTIFY_CTX, "Not enough memory to allocate buffer\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Null-terminate the string

    return realsize;
}

void __spotify_get_device_status(const char *host) {
    CURL *curl;
    CURLcode res;
    struct __spotify_memory_struct chunk = {NULL, 0};

    // Initialize empty response buffer
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        char *url = my_cat3str("http://", host, ":3678/status");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         __spotify_write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            log_error(SPOTIFY_CTX, "curl_easy_perform() failed: %s\n",
                      curl_easy_strerror(res));
            player_set_active(__spotify_player, 0);
            player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
        } else {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 200 && response_code < 300) {
                if (response_code != 200) {
                    player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
                    player_set_active(__spotify_player, 0);
                    log_config(SPOTIFY_CTX,
                               "Device is not active (HTTP %ld).\n",
                               response_code);
                } else if (chunk.size == 0) {
                    player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
                    player_set_active(__spotify_player, 0);
                    log_config(SPOTIFY_CTX, "Device is not active.\n");
                } else {
                    player_set_active(__spotify_player, 1);
                    log_config(SPOTIFY_CTX, "Response: %s\n", chunk.memory);
                    cJSON *response = cJSON_Parse(chunk.memory);
                    if (response) {
                        int paused = 0;
                        int stopped = 0;
                        cJSON *paused_json = cJSON_GetObjectItem(response, "paused");
                        cJSON *stopped_json = cJSON_GetObjectItem(response, "stopped");
                        if (paused_json && cJSON_IsBool(paused_json)) {
                            paused = cJSON_IsTrue(paused_json);
                        }
                        if (stopped_json && cJSON_IsBool(stopped_json)) {
                            stopped = cJSON_IsTrue(stopped_json);
                        }

                        cJSON *track = cJSON_GetObjectItem(response, "track");
                        if (track) {
                            __spotify_fill_info(track);
                        }

                        if (stopped || !track) {
                            player_set_playback_status(__spotify_player,
                                                       PLAYER_PLAYBACK_STOPPED);
                        } else if (paused) {
                            player_set_playback_status(__spotify_player,
                                                       PLAYER_PLAYBACK_PAUSED);
                        } else {
                            player_set_playback_status(__spotify_player,
                                                       PLAYER_PLAYBACK_PLAYING);
                        }
                        cJSON_Delete(response);
                    }
                }
            } else {
                log_warning(SPOTIFY_CTX, "Unexpected status response: %ld\n",
                            response_code);
                player_set_playback_status(__spotify_player, PLAYER_PLAYBACK_STOPPED);
                player_set_active(__spotify_player, 0);
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);

        free (url);
    }

    curl_global_cleanup();
    free (chunk.memory);

}

int __spotify_connect() {
    if (!__spotify_data.context) {
        log_error(SPOTIFY_CTX, "Spotify ws context not initialized\n");
        return 0;
    }

    while (player_thread_running(__spotify_player) && !__spotify_data.connected) {
        struct lws_client_connect_info ccinfo;
        memset(&ccinfo, 0, sizeof(ccinfo));

        ccinfo.context = __spotify_data.context;
        ccinfo.address = __spotify_data.host;
        ccinfo.port = 3678;
        ccinfo.path = "/events";
        ccinfo.host = lws_canonical_hostname(__spotify_data.context);
        ccinfo.origin = ccinfo.host;
        ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;

        log_info(SPOTIFY_CTX, "Trying to connect to %s\n", __spotify_data.host);
        __spotify_data.connected = 0;
        __spotify_data.web_socket = lws_client_connect_via_info(&ccinfo);

        if (__spotify_data.web_socket) {
            int wait_count = 0;
            lws_set_timeout(__spotify_data.web_socket, PENDING_TIMEOUT_AWAITING_CONNECT_RESPONSE, 5);
            while (player_thread_running(__spotify_player) && __spotify_data.web_socket
                   && !__spotify_data.connected && wait_count < 50) {
                lws_service(__spotify_data.context, /* timeout_ms = */ 100);
                wait_count++;
            }

            if (__spotify_data.connected) {
                break;
            }

            log_info(SPOTIFY_CTX, "Connection failed\n");
        } else {
            log_info(SPOTIFY_CTX, "Connection failed\n");
        }

        sleep(1);
    }

    return __spotify_data.connected;
}

int __spotify_init() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    lws_set_log_level(/*LLL_NOTICE | LLL_INFO |*/ LLL_ERR | LLL_WARN, NULL);

    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.timeout_secs = 2;

    __spotify_data.connected = 0;
    __spotify_data.web_socket = NULL;
    __spotify_data.context = lws_create_context(&info);
    if (!__spotify_data.context) {
        log_error(SPOTIFY_CTX, "Failed to create LWS context\n");
        return 0;
    }

    log_config(SPOTIFY_CTX, "Spotify ws context created.\n");

    return 1;
}

int __spotify_run() {
    if (!__spotify_data.connected) {
        log_config(SPOTIFY_CTX, "Trying to (re-)connect to spotify\n");
        if (__spotify_connect()) {
            log_config(SPOTIFY_CTX, "Successfully connected to spotify\n");
            __spotify_get_device_status(__spotify_data.host);
        }
    }

    if (__spotify_data.connected) {
        log_config(SPOTIFY_CTX, "Running spotify thread function.\n");
        if (player_thread_running(__spotify_player) && __spotify_data.web_socket) {
            log_config(SPOTIFY_CTX, "Processing websocket activities\n");
            lws_service(__spotify_data.context, /* timeout_ms = */ 1000);
            log_config(SPOTIFY_CTX, "Done processing websocket activities\n");
        }
    }

    return 1;
}

int __spotify_abort() {
    __spotify_data.connected = 0;
    if (__spotify_data.context) {
        log_config(SPOTIFY_CTX, "Canceling websocket listening\n");
        lws_cancel_service(__spotify_data.context);
        log_config(SPOTIFY_CTX, "Done\n");
    }
    return 1;
}

int __spotify_cleanup() {
    log_config(SPOTIFY_CTX, "Spotify cleanup\n");
    free_and_set_null((void **) &__spotify_cover_url);
    free_and_set_null((void **) &__spotify_cover_path);
    if (__spotify_data.web_socket) {
        lws_callback_on_writable(__spotify_data.web_socket);
    }
    if (__spotify_data.context) {
        lws_context_destroy(__spotify_data.context);
    }
    __spotify_data.context = NULL;
    __spotify_data.web_socket = NULL;
    __spotify_data.connected = 0;
    log_config(SPOTIFY_CTX, "Spotify cleanup done\n");
    return 1;
}

player *spotify_init(char *spotify_host,
                     char *label,
                     char *icon,
                     int check_millis,
                     int show_cover) {
    strncpy(__spotify_data.host, spotify_host, 255);
    __spotify_player = player_new("SPOTIFY",
                                  icon,
                                  label,
                                  check_millis,
                                  &__spotify_init,
                                  &__spotify_run,
                                  &__spotify_cleanup,
                                  &__spotify_abort,
                                  NULL,
                                  NULL);
    return __spotify_player;
}
