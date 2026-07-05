/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "radio_browser.h"

#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RADIO_BROWSER_DEFAULT_SERVER "https://de1.api.radio-browser.info"
#define RADIO_BROWSER_DEFAULT_USER_AGENT "VE301"

typedef struct rb_buffer {
    char *data;
    size_t size;
} rb_buffer;

typedef struct rb_state {
    char *server;
    char *user_agent;
} rb_state;

static rb_state state = {0};

static size_t rb_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    rb_buffer *buffer = (rb_buffer *) userdata;
    char *new_data = realloc(buffer->data, buffer->size + total + 1);
    if (!new_data) {
        return 0;
    }

    buffer->data = new_data;
    memcpy(buffer->data + buffer->size, ptr, total);
    buffer->size += total;
    buffer->data[buffer->size] = 0;
    return total;
}

static char *rb_copy_or_default(const char *value, const char *fallback) {
    if (value && value[0]) {
        return my_copystr(value);
    }
    return my_copystr(fallback);
}

static void rb_buffer_free(rb_buffer *buffer) {
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    buffer->size = 0;
}

static char *rb_join_url(const char *base, const char *path) {
    const char *used_base = base && base[0] ? base : RADIO_BROWSER_DEFAULT_SERVER;
    const char *used_path = path ? path : "";
    size_t base_len = strlen(used_base);
    size_t path_len = strlen(used_path);
    int skip_path_slash = base_len > 0 && used_base[base_len - 1] == '/' && path_len > 0
                          && used_path[0] == '/';
    int need_slash = base_len > 0 && used_base[base_len - 1] != '/' && path_len > 0
                     && used_path[0] != '/';
    size_t len = base_len + path_len + 2;
    char *url = malloc(len);
    if (!url) {
        return NULL;
    }

    snprintf(url, len, "%s%s%s", used_base, need_slash ? "/" : "", skip_path_slash ? used_path + 1 : used_path);
    return url;
}

static char *rb_escape(const char *value) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    char *escaped = value ? curl_easy_escape(curl, value, 0) : NULL;
    curl_easy_cleanup(curl);
    return escaped;
}

static char *rb_fetch_url(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error(MAIN_CTX, "Radio Browser: failed to create curl handle\n");
        return NULL;
    }

    rb_buffer buffer = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rb_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     state.user_agent ? state.user_agent : RADIO_BROWSER_DEFAULT_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error(MAIN_CTX,
                  "Radio Browser request failed for %s: %s\n",
                  url,
                  curl_easy_strerror(res));
        rb_buffer_free(&buffer);
        return NULL;
    }

    if (http_code < 200 || http_code >= 300) {
        log_error(MAIN_CTX, "Radio Browser request failed for %s: HTTP %ld\n", url, http_code);
        rb_buffer_free(&buffer);
        return NULL;
    }

    return buffer.data;
}

static char *rb_fetch_path_once(const char *server, const char *path) {
    char *url = rb_join_url(server, path);
    if (!url) {
        return NULL;
    }

    char *body = rb_fetch_url(url);
    free(url);
    return body;
}

static char *rb_fetch_path(const char *path) {
    const char *primary = state.server && state.server[0] ? state.server : RADIO_BROWSER_DEFAULT_SERVER;
    char *body = rb_fetch_path_once(primary, path);
    if (body) {
        return body;
    }

    if (strcmp(primary, RADIO_BROWSER_DEFAULT_SERVER)) {
        return rb_fetch_path_once(RADIO_BROWSER_DEFAULT_SERVER, path);
    }

    return NULL;
}

static char *rb_json_string(cJSON *obj, const char **keys, unsigned int n_keys) {
    if (!obj || !cJSON_IsObject(obj)) {
        return NULL;
    }

    for (unsigned int i = 0; i < n_keys; i++) {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, keys[i]);
        if (cJSON_IsString(item) && item->valuestring && item->valuestring[0]) {
            return my_copystr(item->valuestring);
        }
    }

    return NULL;
}

static int rb_json_int(cJSON *obj, const char **keys, unsigned int n_keys, int default_value) {
    if (!obj || !cJSON_IsObject(obj)) {
        return default_value;
    }

    for (unsigned int i = 0; i < n_keys; i++) {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, keys[i]);
        if (item && cJSON_IsNumber(item)) {
            return (int) cJSON_GetNumberValue(item);
        }
    }

    return default_value;
}

static radio_browser_entry *rb_entry_from_json(cJSON *obj) {
    if (!obj || !cJSON_IsObject(obj)) {
        return NULL;
    }

    radio_browser_entry *entry = calloc(1, sizeof(radio_browser_entry));
    if (!entry) {
        return NULL;
    }

    const char *label_keys[] = {"name", "tag", "language", "country", "countryname"};
    const char *value_keys[] = {
        "countrycode", "code", "languagecodes", "languagecode", "language", "tag", "name"
    };
    const char *count_keys[] = {"stationcount", "count"};

    entry->label = rb_json_string(obj, label_keys, sizeof(label_keys) / sizeof(label_keys[0]));
    entry->value = rb_json_string(obj, value_keys, sizeof(value_keys) / sizeof(value_keys[0]));
    entry->stationcount = (unsigned int) rb_json_int(obj, count_keys, sizeof(count_keys) / sizeof(count_keys[0]), 0);

    if (!entry->label) {
        entry->label = rb_copy_or_default(entry->value, "Unknown");
    }
    if (!entry->value) {
        entry->value = my_copystr(entry->label);
    }

    return entry;
}

static radio_browser_station *rb_station_from_json(cJSON *obj) {
    if (!obj || !cJSON_IsObject(obj)) {
        return NULL;
    }

    radio_browser_station *station = calloc(1, sizeof(radio_browser_station));
    if (!station) {
        return NULL;
    }

    const char *uuid_keys[] = {"stationuuid", "uuid"};
    const char *name_keys[] = {"name", "title"};
    const char *url_keys[] = {"url", "homepage"};
    const char *resolved_keys[] = {"url_resolved", "resolved_url", "streamurl"};
    const char *homepage_keys[] = {"homepage"};
    const char *favicon_keys[] = {"favicon", "icon"};
    const char *tags_keys[] = {"tags"};
    const char *country_keys[] = {"countrycode", "country"};
    const char *language_keys[] = {"language", "languagecodes", "languagecode"};
    const char *languagecodes_keys[] = {"languagecodes", "languagecode", "language"};
    const char *codec_keys[] = {"codec"};
    const char *bitrate_keys[] = {"bitrate"};
    const char *lastcheckok_keys[] = {"lastcheckok"};

    station->stationuuid = rb_json_string(obj, uuid_keys, sizeof(uuid_keys) / sizeof(uuid_keys[0]));
    station->name = rb_json_string(obj, name_keys, sizeof(name_keys) / sizeof(name_keys[0]));
    station->url = rb_json_string(obj, url_keys, sizeof(url_keys) / sizeof(url_keys[0]));
    station->url_resolved = rb_json_string(obj,
                                           resolved_keys,
                                           sizeof(resolved_keys) / sizeof(resolved_keys[0]));
    station->homepage = rb_json_string(obj,
                                       homepage_keys,
                                       sizeof(homepage_keys) / sizeof(homepage_keys[0]));
    station->favicon = rb_json_string(obj, favicon_keys, sizeof(favicon_keys) / sizeof(favicon_keys[0]));
    station->tags = rb_json_string(obj, tags_keys, sizeof(tags_keys) / sizeof(tags_keys[0]));
    station->countrycode = rb_json_string(obj,
                                          country_keys,
                                          sizeof(country_keys) / sizeof(country_keys[0]));
    station->language = rb_json_string(obj,
                                       language_keys,
                                       sizeof(language_keys) / sizeof(language_keys[0]));
    station->languagecodes = rb_json_string(obj,
                                            languagecodes_keys,
                                            sizeof(languagecodes_keys) / sizeof(languagecodes_keys[0]));
    station->codec = rb_json_string(obj, codec_keys, sizeof(codec_keys) / sizeof(codec_keys[0]));
    station->bitrate = rb_json_int(obj, bitrate_keys, sizeof(bitrate_keys) / sizeof(bitrate_keys[0]), 0);
    station->lastcheckok = rb_json_int(obj,
                                       lastcheckok_keys,
                                       sizeof(lastcheckok_keys) / sizeof(lastcheckok_keys[0]),
                                       0);

    if (!station->name) {
        station->name = rb_copy_or_default(station->url_resolved, "Unknown");
    }
    if (!station->url_resolved) {
        station->url_resolved = rb_copy_or_default(station->url, "");
    }
    if (!station->url) {
        station->url = rb_copy_or_default(station->url_resolved, "");
    }

    return station;
}

static int rb_entry_list_append(radio_browser_entry_list *list, radio_browser_entry *entry) {
    radio_browser_entry **new_entries =
        realloc(list->entries, (list->n_entries + 1) * sizeof(radio_browser_entry *));
    if (!new_entries) {
        return 0;
    }
    list->entries = new_entries;
    list->entries[list->n_entries++] = entry;
    return 1;
}

static int rb_station_list_append(radio_browser_station_list *list, radio_browser_station *station) {
    radio_browser_station **new_stations =
        realloc(list->stations, (list->n_stations + 1) * sizeof(radio_browser_station *));
    if (!new_stations) {
        return 0;
    }
    list->stations = new_stations;
    list->stations[list->n_stations++] = station;
    return 1;
}

static radio_browser_entry_list *rb_parse_entry_list(const char *json_body) {
    cJSON *json = cJSON_Parse(json_body);
    if (!json) {
        log_error(MAIN_CTX, "Radio Browser: could not parse entry JSON\n");
        return NULL;
    }

    if (!cJSON_IsArray(json)) {
        cJSON_Delete(json);
        return NULL;
    }

    radio_browser_entry_list *list = calloc(1, sizeof(radio_browser_entry_list));
    if (!list) {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        radio_browser_entry *entry = rb_entry_from_json(item);
        if (!entry) {
            continue;
        }
        if (!rb_entry_list_append(list, entry)) {
            radio_browser_entry_free(entry);
            break;
        }
    }

    cJSON_Delete(json);
    return list;
}

static radio_browser_station_list *rb_parse_station_list(const char *json_body) {
    cJSON *json = cJSON_Parse(json_body);
    if (!json) {
        log_error(MAIN_CTX, "Radio Browser: could not parse station JSON\n");
        return NULL;
    }

    if (!cJSON_IsArray(json)) {
        cJSON_Delete(json);
        return NULL;
    }

    radio_browser_station_list *list = calloc(1, sizeof(radio_browser_station_list));
    if (!list) {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        radio_browser_station *station = rb_station_from_json(item);
        if (!station) {
            continue;
        }
        if (!rb_station_list_append(list, station)) {
            radio_browser_station_free(station);
            break;
        }
    }

    cJSON_Delete(json);
    return list;
}

static char *rb_json_root_string(cJSON *json, const char **keys, unsigned int n_keys) {
    if (!json) {
        return NULL;
    }

    if (cJSON_IsArray(json)) {
        cJSON *first = cJSON_GetArrayItem(json, 0);
        if (first) {
            char *res = rb_json_root_string(first, keys, n_keys);
            if (res) {
                return res;
            }
        }
    }

    if (cJSON_IsString(json) && json->valuestring && json->valuestring[0]) {
        return my_copystr(json->valuestring);
    }

    if (!cJSON_IsObject(json)) {
        return NULL;
    }

    return rb_json_string(json, keys, n_keys);
}

static char *rb_fetch_stream_url_from_stationuuid(const char *stationuuid) {
    char *escaped_uuid = rb_escape(stationuuid);
    if (!escaped_uuid) {
        return NULL;
    }

    size_t path_len = strlen("/json/url/") + strlen(escaped_uuid) + 1;
    char *path = malloc(path_len);
    if (!path) {
        curl_free(escaped_uuid);
        return NULL;
    }
    snprintf(path, path_len, "/json/url/%s", escaped_uuid);
    curl_free(escaped_uuid);

    char *json_body = rb_fetch_path(path);
    free(path);
    if (!json_body) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(json_body);
    free(json_body);
    if (!json) {
        return NULL;
    }

    const char *url_keys[] = {"url_resolved", "url", "streamurl", "resolved_url"};
    char *resolved = rb_json_root_string(json, url_keys, sizeof(url_keys) / sizeof(url_keys[0]));
    cJSON_Delete(json);
    return resolved;
}

int radio_browser_init(const char *user_agent, const char *preferred_server) {
    free_and_set_null((void **) &state.server);
    free_and_set_null((void **) &state.user_agent);

    state.server = rb_copy_or_default(preferred_server, RADIO_BROWSER_DEFAULT_SERVER);
    state.user_agent = rb_copy_or_default(user_agent, RADIO_BROWSER_DEFAULT_USER_AGENT);
    return 1;
}

void radio_browser_cleanup(void) {
    free_and_set_null((void **) &state.server);
    free_and_set_null((void **) &state.user_agent);
}

static radio_browser_entry_list *rb_get_entries(const char *path) {
    char *json_body = rb_fetch_path(path);
    if (!json_body) {
        return NULL;
    }

    radio_browser_entry_list *list = rb_parse_entry_list(json_body);
    free(json_body);
    return list;
}

static radio_browser_station_list *rb_get_stations(const char *path) {
    char *json_body = rb_fetch_path(path);
    if (!json_body) {
        return NULL;
    }

    radio_browser_station_list *list = rb_parse_station_list(json_body);
    free(json_body);
    return list;
}

radio_browser_entry_list *radio_browser_get_local_entries(const char *countrycode, unsigned int limit) {
    char *escaped_country = rb_escape(countrycode ? countrycode : "");
    if (!escaped_country) {
        return NULL;
    }

    char path[1024];
    snprintf(path,
             sizeof(path),
             "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=%u&countrycode=%s",
             limit,
             escaped_country);
    curl_free(escaped_country);
    return rb_get_entries(path);
}

radio_browser_entry_list *radio_browser_get_tags(unsigned int limit) {
    char path[512];
    snprintf(path,
             sizeof(path),
             "/json/tags?order=stationcount&reverse=true&hidebroken=true&limit=%u",
             limit);
    return rb_get_entries(path);
}

radio_browser_entry_list *radio_browser_get_languages(unsigned int limit) {
    char path[512];
    snprintf(path,
             sizeof(path),
             "/json/languages?order=stationcount&reverse=true&hidebroken=true&limit=%u",
             limit);
    return rb_get_entries(path);
}

radio_browser_station_list *radio_browser_get_local_stations(const char *countrycode,
                                                             unsigned int limit) {
    char *escaped_country = rb_escape(countrycode ? countrycode : "");
    if (!escaped_country) {
        return NULL;
    }

    char path[1024];
    snprintf(path,
             sizeof(path),
             "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=%u&countrycode=%s",
             limit,
             escaped_country);
    curl_free(escaped_country);
    return rb_get_stations(path);
}

radio_browser_station_list *radio_browser_get_stations_by_tag(const char *tag, unsigned int limit) {
    char *escaped_tag = rb_escape(tag ? tag : "");
    if (!escaped_tag) {
        return NULL;
    }

    char path[1024];
    snprintf(path,
             sizeof(path),
             "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=%u&tag=%s",
             limit,
             escaped_tag);
    curl_free(escaped_tag);
    return rb_get_stations(path);
}

radio_browser_station_list *radio_browser_get_stations_by_language(const char *language,
                                                                   unsigned int limit) {
    char *escaped_language = rb_escape(language ? language : "");
    if (!escaped_language) {
        return NULL;
    }

    char path[1024];
    const char *param = "language";
    if (language && strlen(language) <= 5 && strchr(language, ' ') == NULL) {
        param = "languagecode";
    }
    snprintf(path,
             sizeof(path),
             "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=%u&%s=%s",
             limit,
             param,
             escaped_language);
    curl_free(escaped_language);
    return rb_get_stations(path);
}

char *radio_browser_resolve_stream_url(const radio_browser_station *station) {
    if (!station) {
        return NULL;
    }

    char *resolved = NULL;
    if (station->stationuuid && station->stationuuid[0]) {
        resolved = rb_fetch_stream_url_from_stationuuid(station->stationuuid);
    }

    if (!resolved || !resolved[0]) {
        free(resolved);
        resolved = rb_copy_or_default(station->url_resolved, station->url);
    }

    if (!resolved || !resolved[0]) {
        free(resolved);
        return NULL;
    }

    return resolved;
}

void radio_browser_entry_free(radio_browser_entry *entry) {
    if (!entry) {
        return;
    }

    free_and_set_null((void **) &entry->label);
    free_and_set_null((void **) &entry->value);
    free(entry);
}

radio_browser_entry *radio_browser_entry_clone(const radio_browser_entry *entry) {
    if (!entry) {
        return NULL;
    }

    radio_browser_entry *copy = calloc(1, sizeof(radio_browser_entry));
    if (!copy) {
        return NULL;
    }

    copy->label = my_copystr(entry->label);
    copy->value = my_copystr(entry->value);
    copy->stationcount = entry->stationcount;
    return copy;
}

void radio_browser_entry_list_free(radio_browser_entry_list *list) {
    if (!list) {
        return;
    }

    for (unsigned int i = 0; i < list->n_entries; i++) {
        radio_browser_entry_free(list->entries[i]);
    }
    free(list->entries);
    free(list);
}

void radio_browser_station_free(radio_browser_station *station) {
    if (!station) {
        return;
    }

    free_and_set_null((void **) &station->stationuuid);
    free_and_set_null((void **) &station->name);
    free_and_set_null((void **) &station->url);
    free_and_set_null((void **) &station->url_resolved);
    free_and_set_null((void **) &station->homepage);
    free_and_set_null((void **) &station->favicon);
    free_and_set_null((void **) &station->tags);
    free_and_set_null((void **) &station->countrycode);
    free_and_set_null((void **) &station->language);
    free_and_set_null((void **) &station->languagecodes);
    free_and_set_null((void **) &station->codec);
    free(station);
}

radio_browser_station *radio_browser_station_clone(const radio_browser_station *station) {
    if (!station) {
        return NULL;
    }

    radio_browser_station *copy = calloc(1, sizeof(radio_browser_station));
    if (!copy) {
        return NULL;
    }

    copy->stationuuid = my_copystr(station->stationuuid);
    copy->name = my_copystr(station->name);
    copy->url = my_copystr(station->url);
    copy->url_resolved = my_copystr(station->url_resolved);
    copy->homepage = my_copystr(station->homepage);
    copy->favicon = my_copystr(station->favicon);
    copy->tags = my_copystr(station->tags);
    copy->countrycode = my_copystr(station->countrycode);
    copy->language = my_copystr(station->language);
    copy->languagecodes = my_copystr(station->languagecodes);
    copy->codec = my_copystr(station->codec);
    copy->bitrate = station->bitrate;
    copy->lastcheckok = station->lastcheckok;
    return copy;
}

void radio_browser_station_list_free(radio_browser_station_list *list) {
    if (!list) {
        return;
    }

    for (unsigned int i = 0; i < list->n_stations; i++) {
        radio_browser_station_free(list->stations[i]);
    }
    free(list->stations);
    free(list);
}
