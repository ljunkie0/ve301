#define _GNU_SOURCE
#include "spotify.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPOTIFY_MAX_CONNECT_TRIES 10

static struct lws *web_socket = NULL;

static char *__spotify_host = NULL;
static pthread_t __spotify_thread = 0;
static int __spotify_thread_run = 0;

player *__spotify_player;

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
    player_set_cover_uri(__spotify_player,
                         album_cover_url ? album_cover_url->valuestring : NULL);

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

static int __spotify_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {

    if (reason == LWS_CALLBACK_CLIENT_ESTABLISHED) {
        log_config(SPOTIFY_CTX, "WebSocket connected\n");
    } else if (reason == LWS_CALLBACK_CLIENT_RECEIVE) {
        char *message = (char *)in;
        if (message && strlen(message) > 0) {
            log_config(SPOTIFY_CTX, "Received message: %s\n", message);
            cJSON *event = cJSON_Parse(message);
            if (event == NULL) {
                log_error(SPOTIFY_CTX, "Error parsing JSON\n");
                return 0;
            }
            log_config(SPOTIFY_CTX, "Event: %p\n", event);
            cJSON *type = cJSON_GetObjectItem(event, "type");
            log_config(SPOTIFY_CTX, "Type: %s\n", type->valuestring);
            // Process the events based on their content
            if (strstr(type->valuestring, "inactive")) {
                log_config(SPOTIFY_CTX, "Device is now inactive.\n");
                player_set_status(__spotify_player, 0);
            } else if (strstr(type->valuestring, "active")) {
                log_config(SPOTIFY_CTX, "Device is now active.\n");
                player_set_status(__spotify_player, 1);
            } else if (strstr(type->valuestring, "metadata")) {
                log_config(SPOTIFY_CTX, "Metadata changed.\n");
                cJSON *metadata = cJSON_GetObjectItem(event, "data");
                if (metadata != NULL) {
                    __spotify_fill_info(metadata);
                }

                cJSON_Delete(event);
            }
        }
    } else if (reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR) {
        log_error(SPOTIFY_CTX, "Connection error: %s\n", (char *)in);
        web_socket = NULL;
    } else if (reason == LWS_CALLBACK_CLIENT_CLOSED) {
        log_info(SPOTIFY_CTX, "WebSocket connection closed\n");
    }
    return 0;
}

enum protocols { PROTOCOL_EXAMPLE = 0, PROTOCOL_COUNT };

static struct lws_protocols protocols[] = {
    {
        .name = "ws",                   /* Protocol name*/
                      .callback = __spotify_callback, /* Protocol callback */
        .per_session_data_size = 0,     /* Protocol callback 'userdata' size */
        .rx_buffer_size = 0, /* Receve buffer size (0 = no restriction) */
        .id = 0,             /* Protocol Id (version) (optional) */
        .user = NULL, /* 'User data' ptr, to access in 'protocol callback */
        .tx_packet_size =
        0 /* Transmission buffer size restriction (0 = no restriction) */
    },
    {NULL, NULL, 0, 0, 0, NULL, 0} /* terminator */
};

void *__spotify_thread_func(void *) {
    log_config(SPOTIFY_CTX, "Spotify thread starting\n");
    pthread_setname_np(pthread_self(), "Spotify thead");
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.timeout_secs = 2;

    struct lws_context *context = lws_create_context(&info);

    time_t old = 0;
    time_t old_conn_check = 0;
    int connection_tries = 0;
    while (__spotify_thread_run) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        if (__spotify_thread_run && tv.tv_sec != old) {
            /* Connect if we are not connected to the server. */
            if (__spotify_thread_run && !web_socket && tv.tv_sec - old_conn_check > 5) {
                if (++connection_tries <= SPOTIFY_MAX_CONNECT_TRIES) {
                    connection_tries++;
                    struct lws_client_connect_info ccinfo;
                    memset(&ccinfo, 0, sizeof(ccinfo));

                    ccinfo.context = context;
                    ccinfo.address = __spotify_host;
                    ccinfo.port = 3678;
                    ccinfo.path = "/events";
                    ccinfo.host = lws_canonical_hostname(context);
                    ccinfo.origin = ccinfo.host;
                    ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;

                    log_info(SPOTIFY_CTX, "Trying to connect to %s\n", __spotify_host);
                    web_socket = lws_client_connect_via_info(&ccinfo);

                    if (web_socket) {
                        log_info(SPOTIFY_CTX, "Connection successfull\n");
                        lws_set_timeout(web_socket, PENDING_TIMEOUT_AWAITING_CONNECT_RESPONSE, 1);
                    } else {
                        log_info(SPOTIFY_CTX, "Connection failed\n");
                    }

                } else {
                    log_warning(SPOTIFY_CTX,
                                "Could not establish connection to spotify after %d "
                                "tries. Giving up.\n",
                                SPOTIFY_MAX_CONNECT_TRIES);
                    __spotify_thread_run = 0;
                }

                old_conn_check = tv.tv_sec;
            }
        }

        if (__spotify_thread_run && web_socket) {
            log_info(SPOTIFY_CTX, "Processing websocket activities\n");
            lws_service(context, /* timeout_ms = */ 1000);
            log_info(SPOTIFY_CTX, "Done processing websocket activities\n");
        }

        old = tv.tv_sec;
    }

    log_info(SPOTIFY_CTX, "Spotify thread finished. Destroying LWS context\n");
    lws_context_destroy(context);

    return NULL;
}

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
        printf("Not enough memory to allocate buffer\n");
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
    struct __spotify_memory_struct chunk = {0};

    // Initialize empty response buffer
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        char *url = my_cat3str("http://", host, ":3678/status");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         __spotify_write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            log_error(SPOTIFY_CTX, "curl_easy_perform() failed: %s\n",
                      curl_easy_strerror(res));
        } else if (chunk.size == 0) {
            player_set_status(__spotify_player, 0);
            log_config(SPOTIFY_CTX, "Device is not active.\n");
        } else {
            player_set_status(__spotify_player, 1);
            log_config(SPOTIFY_CTX, "Response: %s\n", chunk.memory);
            cJSON *response = cJSON_Parse(chunk.memory);
            if (response) {
                cJSON *track = cJSON_GetObjectItem(response, "track");
                __spotify_fill_info(track);
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);

        free (url);
    }

    curl_global_cleanup();
    free (chunk.memory);
}

void __start_spotify_thread(char *spotify_host) {

    if (__spotify_thread_run) {
        log_warning(SPOTIFY_CTX, "Spotify thread already started\n");
        return;
    }

    __spotify_host = strdup(spotify_host);
    __spotify_get_device_status(spotify_host);
    __spotify_thread_run = 1;
    int r = pthread_create(&__spotify_thread, NULL, __spotify_thread_func, NULL);
    if (r) {
        __spotify_thread_run = 0;
        log_error(BASE_CTX, "Could not start spotify thread: %d\n", r);
    }
}

void __stop_spotify_thread() {
    if (__spotify_thread_run) {
        __spotify_thread_run = 0;
        void *res;
        pthread_join(__spotify_thread, &res);
    }
}

player *spotify_init(char *spotify_host, char *label, char *icon,
                     int check_seconds) {
    __spotify_player = player_new("SPOTIFY", icon, label, check_seconds, NULL);
    __start_spotify_thread(spotify_host);
    return __spotify_player;
}

void spotify_close() {
    log_info(SPOTIFY_CTX, "Closing ...\n");
    __stop_spotify_thread();
    player_free(__spotify_player);
    log_info(SPOTIFY_CTX, "Closing Done\n");
}
