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

#include "podcast.h"

#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"

#include <ctype.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PODCAST_DEFAULT_USER_AGENT "VE301"

typedef struct podcast_buffer {
    char *data;
    size_t size;
} podcast_buffer;

static size_t podcast_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    podcast_buffer *buffer = (podcast_buffer *) userdata;
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

static void podcast_buffer_free(podcast_buffer *buffer) {
    if (buffer && buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    if (buffer) {
        buffer->size = 0;
    }
}

static char *podcast_copy_or_default(const char *value, const char *fallback) {
    if (value && value[0]) {
        return my_copystr(value);
    }
    return my_copystr(fallback);
}

static char *podcast_trim(char *str) {
    if (!str) {
        return NULL;
    }

    while (*str && isspace((unsigned char) *str)) {
        str++;
    }

    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char) *(end - 1))) {
        end--;
    }
    *end = 0;
    return str;
}

static int podcast_is_comment_or_blank(const char *line) {
    if (!line) {
        return 1;
    }

    while (*line && isspace((unsigned char) *line)) {
        line++;
    }

    return !*line || *line == '#';
}

static int podcast_starts_with(const char *value, const char *prefix) {
    if (!value || !prefix) {
        return 0;
    }
    return strncmp(value, prefix, strlen(prefix)) == 0;
}

static int podcast_month_number(const char *month) {
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };

    if (!month) {
        return 0;
    }

    for (int i = 0; i < 12; i++) {
        int matches = 1;
        for (int c = 0; c < 3; c++) {
            if (tolower((unsigned char) month[c])
                != tolower((unsigned char) months[i][c])) {
                matches = 0;
                break;
            }
        }
        if (matches) {
            return i + 1;
        }
    }

    return 0;
}

static char *podcast_format_date(const char *date) {
    if (!date || !date[0]) {
        return NULL;
    }

    if (strlen(date) >= 10 && isdigit((unsigned char) date[0])
        && isdigit((unsigned char) date[1]) && isdigit((unsigned char) date[2])
        && isdigit((unsigned char) date[3]) && date[4] == '-'
        && isdigit((unsigned char) date[5]) && isdigit((unsigned char) date[6])
        && date[7] == '-' && isdigit((unsigned char) date[8])
        && isdigit((unsigned char) date[9])) {
        char buffer[11];
        snprintf(buffer, sizeof(buffer), "%c%c.%c%c.%c%c%c%c",
                 date[8], date[9], date[5], date[6], date[0], date[1], date[2], date[3]);
        return my_copystr(buffer);
    }

    const char *value = strchr(date, ',');
    value = value ? value + 1 : date;
    while (*value && isspace((unsigned char) *value)) {
        value++;
    }

    int day = 0;
    int year = 0;
    char month[4] = {0};
    if (sscanf(value, "%d %3s %d", &day, month, &year) == 3) {
        int month_number = podcast_month_number(month);
        if (month_number > 0) {
            char buffer[11];
            snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", day, month_number, year);
            return my_copystr(buffer);
        }
    }

    return my_copystr(date);
}

static char *podcast_fetch_url(const char *url) {
    if (!url || !url[0]) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error(MAIN_CTX, "Podcast: failed to create curl handle\n");
        return NULL;
    }

    podcast_buffer buffer = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, podcast_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, PODCAST_DEFAULT_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error(MAIN_CTX, "Podcast request failed for %s: %s\n", url, curl_easy_strerror(res));
        podcast_buffer_free(&buffer);
        return NULL;
    }

    if (http_code < 200 || http_code >= 300) {
        log_error(MAIN_CTX, "Podcast request failed for %s: HTTP %ld\n", url, http_code);
        podcast_buffer_free(&buffer);
        return NULL;
    }

    return buffer.data;
}

static char *podcast_node_prop_dup(xmlNodePtr node, const char *name) {
    if (!node || !name) {
        return NULL;
    }

    xmlChar *value = xmlGetProp(node, BAD_CAST name);
    if (!value) {
        return NULL;
    }

    char *copy = my_copystr((const char *) value);
    xmlFree(value);
    return copy;
}

static char *podcast_node_child_text_dup(xmlNodePtr node, const char *name) {
    if (!node || !name) {
        return NULL;
    }

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcasecmp(child->name, BAD_CAST name) == 0) {
            xmlChar *content = xmlNodeGetContent(child);
            if (!content) {
                return NULL;
            }

            char *copy = my_copystr((const char *) content);
            xmlFree(content);
            return copy;
        }
    }

    return NULL;
}

static int podcast_feed_list_append(podcast_feed_list *list, podcast_feed *feed) {
    podcast_feed **new_feeds = realloc(list->feeds, (list->n_feeds + 1) * sizeof(podcast_feed *));
    if (!new_feeds) {
        return 0;
    }

    list->feeds = new_feeds;
    list->feeds[list->n_feeds++] = feed;
    return 1;
}

static int podcast_episode_list_append(podcast_episode_list *list, podcast_episode *episode) {
    podcast_episode **new_episodes =
        realloc(list->episodes, (list->n_episodes + 1) * sizeof(podcast_episode *));
    if (!new_episodes) {
        return 0;
    }

    list->episodes = new_episodes;
    list->episodes[list->n_episodes++] = episode;
    return 1;
}

static podcast_feed *podcast_feed_new(const char *name, const char *url) {
    podcast_feed *feed = calloc(1, sizeof(podcast_feed));
    if (!feed) {
        return NULL;
    }

    feed->name = podcast_copy_or_default(name, "Podcast");
    feed->url = podcast_copy_or_default(url, NULL);
    if (!feed->url) {
        podcast_feed_free(feed);
        return NULL;
    }

    return feed;
}

static podcast_episode *podcast_episode_new(const char *name, const char *url, const char *date) {
    podcast_episode *episode = calloc(1, sizeof(podcast_episode));
    if (!episode) {
        return NULL;
    }

    episode->name = podcast_copy_or_default(name, "Episode");
    episode->date = podcast_format_date(date);
    episode->url = podcast_copy_or_default(url, NULL);
    if (!episode->url) {
        podcast_episode_free(episode);
        return NULL;
    }

    return episode;
}

podcast_feed_list *podcast_feed_list_load(const char *path) {
    if (!path || !path[0]) {
        return NULL;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        log_error(MAIN_CTX, "Podcast: could not open feed file %s\n", path);
        return NULL;
    }

    podcast_feed_list *list = calloc(1, sizeof(podcast_feed_list));
    if (!list) {
        fclose(f);
        return NULL;
    }

    char *line = NULL;
    size_t line_cap = 0;
    while (getline(&line, &line_cap, f) != -1) {
        char *trimmed = podcast_trim(line);
        if (podcast_is_comment_or_blank(trimmed)) {
            continue;
        }

        char *sep = strchr(trimmed, '|');
        if (!sep) {
            continue;
        }

        *sep = 0;
        char *name = podcast_trim(trimmed);
        char *url = podcast_trim(sep + 1);
        if (!url || !url[0]) {
            continue;
        }

        podcast_feed *feed = podcast_feed_new(name && name[0] ? name : url, url);
        if (!feed) {
            continue;
        }

        if (!podcast_feed_list_append(list, feed)) {
            podcast_feed_free(feed);
            break;
        }
    }

    free(line);
    fclose(f);
    return list;
}

static podcast_episode *podcast_episode_from_rss_item(xmlNodePtr item) {
    char *name = podcast_node_child_text_dup(item, "title");
    char *date = podcast_node_child_text_dup(item, "pubDate");
    char *url = NULL;

    for (xmlNodePtr child = item->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcasecmp(child->name, BAD_CAST "enclosure") == 0) {
            char *type = podcast_node_prop_dup(child, "type");
            char *candidate = podcast_node_prop_dup(child, "url");
            if (candidate && candidate[0]
                && (!type || podcast_starts_with(type, "audio/")
                    || strcmp(type, "application/octet-stream") == 0)) {
                url = candidate;
                free(type);
                break;
            }
            free(candidate);
            free(type);
        }
    }

    if (!url || !url[0]) {
        free(name);
        free(date);
        free(url);
        return NULL;
    }

    podcast_episode *episode = podcast_episode_new(name, url, date);
    free(name);
    free(date);
    free(url);
    return episode;
}

static int podcast_atom_link_is_audio(xmlNodePtr link) {
    char *rel = podcast_node_prop_dup(link, "rel");
    char *type = podcast_node_prop_dup(link, "type");
    int is_audio = 0;

    if (rel && strcmp(rel, "enclosure") == 0) {
        is_audio = 1;
    } else if (!rel) {
        if (!type || podcast_starts_with(type, "audio/")
            || strcmp(type, "application/octet-stream") == 0) {
            is_audio = 1;
        }
    }

    free(rel);
    free(type);
    return is_audio;
}

static podcast_episode *podcast_episode_from_atom_entry(xmlNodePtr entry) {
    char *name = podcast_node_child_text_dup(entry, "title");
    char *date = podcast_node_child_text_dup(entry, "published");
    if (!date || !date[0]) {
        free(date);
        date = podcast_node_child_text_dup(entry, "updated");
    }
    char *url = NULL;

    for (xmlNodePtr child = entry->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcasecmp(child->name, BAD_CAST "link") == 0) {
            char *candidate = podcast_node_prop_dup(child, "href");
            if (candidate && candidate[0] && podcast_atom_link_is_audio(child)) {
                url = candidate;
                break;
            }
            free(candidate);
        }
    }

    if (!url || !url[0]) {
        free(name);
        free(date);
        free(url);
        return NULL;
    }

    podcast_episode *episode = podcast_episode_new(name, url, date);
    free(name);
    free(date);
    free(url);
    return episode;
}

static podcast_episode_list *podcast_episode_list_from_doc(xmlDocPtr doc, unsigned int limit) {
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        return NULL;
    }

    xmlNodePtr container = root;
    if (xmlStrcasecmp(root->name, BAD_CAST "rss") == 0) {
        for (xmlNodePtr child = root->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE
                && xmlStrcasecmp(child->name, BAD_CAST "channel") == 0) {
                container = child;
                break;
            }
        }
    }

    podcast_episode_list *list = calloc(1, sizeof(podcast_episode_list));
    if (!list) {
        return NULL;
    }

    for (xmlNodePtr child = container->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcasecmp(child->name, BAD_CAST "item") == 0) {
            podcast_episode *episode = podcast_episode_from_rss_item(child);
            if (!episode) {
                continue;
            }
            if (!podcast_episode_list_append(list, episode)) {
                podcast_episode_free(episode);
                break;
            }
        } else if (xmlStrcasecmp(child->name, BAD_CAST "entry") == 0) {
            podcast_episode *episode = podcast_episode_from_atom_entry(child);
            if (!episode) {
                continue;
            }
            if (!podcast_episode_list_append(list, episode)) {
                podcast_episode_free(episode);
                break;
            }
        }

        if (limit > 0 && list->n_episodes >= limit) {
            break;
        }
    }

    return list;
}

podcast_episode_list *podcast_episode_list_load(const char *url, unsigned int limit) {
    if (!url || !url[0]) {
        return NULL;
    }

    char *body = podcast_fetch_url(url);
    if (!body) {
        return NULL;
    }

    xmlDocPtr doc = xmlReadMemory(body,
                                  (int) strlen(body),
                                  url,
                                  NULL,
                                  XML_PARSE_NONET | XML_PARSE_NOBLANKS | XML_PARSE_NOERROR
                                      | XML_PARSE_NOWARNING);
    free(body);
    if (!doc) {
        log_error(MAIN_CTX, "Podcast: could not parse XML from %s\n", url);
        return NULL;
    }

    podcast_episode_list *list = podcast_episode_list_from_doc(doc, limit);
    xmlFreeDoc(doc);
    return list;
}

void podcast_feed_free(podcast_feed *feed) {
    if (!feed) {
        return;
    }

    free_and_set_null((void **) &feed->name);
    free_and_set_null((void **) &feed->url);
    free(feed);
}

void podcast_feed_list_free(podcast_feed_list *list) {
    if (!list) {
        return;
    }

    for (unsigned int i = 0; i < list->n_feeds; i++) {
        podcast_feed_free(list->feeds[i]);
    }
    free(list->feeds);
    free(list);
}

void podcast_episode_free(podcast_episode *episode) {
    if (!episode) {
        return;
    }

    free_and_set_null((void **) &episode->name);
    free_and_set_null((void **) &episode->date);
    free_and_set_null((void **) &episode->url);
    free(episode);
}

void podcast_episode_list_free(podcast_episode_list *list) {
    if (!list) {
        return;
    }

    for (unsigned int i = 0; i < list->n_episodes; i++) {
        podcast_episode_free(list->episodes[i]);
    }
    free(list->episodes);
    free(list);
}
