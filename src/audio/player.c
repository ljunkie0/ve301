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
#include "player.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_COVER_URL_LENGTH 512

typedef struct __player_event_node {
    player_event *event;
    struct __player_event_node *next;
} __player_event_node;

typedef struct __player_action {
    player *owner;
    player_action *action;
    void *data;
} __player_action;

typedef struct __player_action_node {
    __player_action *action;
    struct __player_action_node *next;
} __player_action_node;

void __player_enqueue_action(player *owner, player_action *action_func, void *data);
__player_action *player_next_action(player *owner);
void player_action_free(__player_action *action);

typedef struct __player_internal {
    player player;
    pthread_t player_thread;
    int thread_running;
    player_action *playback_start_function;
    player_action *playback_stop_function;
    player_init_function *init_function;
    player_run_function *run_function;
    player_cleanup_function *cleanup_function;
    player_abort_function *abort_function;
} __player_internal;

static pthread_mutex_t __player_event_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t __player_action_mutex = PTHREAD_MUTEX_INITIALIZER;
static __player_event_node *__player_event_head = NULL;
static __player_event_node *__player_event_tail = NULL;
static __player_action_node *__player_action_head = NULL;
static __player_action_node *__player_action_tail = NULL;

player *player_new(const char *name,
                   const char *icon,
                   const char *label,
                   int check_millis,
                   player_init_function *init_function,
                   player_run_function *run_function,
                   player_cleanup_function *cleanup_function,
                   player_abort_function *abort_function,
                   player_action *playback_start_function,
                   player_action *playback_stop_function) {
    __player_internal *p_internal = malloc(sizeof(__player_internal));
    p_internal->player.name = my_copystr(name);
    p_internal->player.active = 0;
    p_internal->player.previous_active = 0;
    p_internal->player.playback_status = PLAYER_PLAYBACK_UNKNOWN;
    p_internal->player.icon = my_copystr(icon);
    p_internal->player.label = my_copystr(label);
    p_internal->player.album = NULL;
    p_internal->player.artist = NULL;
    p_internal->player.title = NULL;
    p_internal->player.cover_uri = NULL;
    p_internal->player.info_bg_image_path = NULL;
    p_internal->player.check_interval = time_check_interval_new(check_millis);
    p_internal->player.cover_changed = 0;

    p_internal->playback_start_function = playback_start_function;
    p_internal->playback_stop_function = playback_stop_function;
    p_internal->init_function = init_function;
    p_internal->run_function = run_function;
    p_internal->cleanup_function = cleanup_function;
    p_internal->abort_function = abort_function;
    p_internal->player_thread = 0;
    p_internal->thread_running = 0;

    return (player *) p_internal;
}

void player_free(player *p) {


    if (!p) {
        return;
    }

    free_and_set_null((void **) &p->album);
    free_and_set_null((void **) &p->artist);
    free_and_set_null((void **) &p->title);
    free_and_set_null((void **) &p->cover_uri);
    free_and_set_null((void **) &p->name);
    free_and_set_null((void **) &p->label);
    free_and_set_null((void **) &p->icon);
    free_and_set_null((void **) &p->info_bg_image_path);
    if (p->check_interval) {
        time_check_interval_free(p->check_interval);
        p->check_interval = NULL;
    }
    free (p);

}

static void player_listener(player_event event) {
    __player_event_node *node = NULL;

    if (!event.player) {
        return;
    }

    node = malloc(sizeof(__player_event_node));
    if (!node) {
        log_error(AUDIO_CTX,
                  "Could not allocate player event node for %s\n",
                  player_get_name(event.player));
        return;
    }

    node->event = malloc(sizeof(player_event));
    if (!node->event) {
        log_error(AUDIO_CTX,
                  "Could not allocate player event for %s\n",
                  player_get_name(event.player));
        free(node);
        return;
    }

    *(node->event) = event;
    node->next = NULL;

    pthread_mutex_lock(&__player_event_mutex);
    if (__player_event_tail) {
        __player_event_tail->next = node;
    } else {
        __player_event_head = node;
    }
    __player_event_tail = node;
    pthread_mutex_unlock(&__player_event_mutex);
}

static void player_emit_event(player *p, player_status status) {
    if (p) {
        player_event event = {p, status};
        player_listener(event);
    }
}

long player_get_song_id(player *p) {
    return p->song_id;
}

void player_set_song_id(player *p, long song_id) {
    p->song_id = song_id;
}

void player_set_active(player *p, int active) {
    if (p->active == active) {
        return;
    }
    p->active = active;
    p->playback_status = PLAYER_PLAYBACK_STOPPED;
    if (!active) {
        p->playback_status = PLAYER_PLAYBACK_STOPPED;
        free_and_set_null((void **) &p->album);
        free_and_set_null((void **) &p->artist);
        free_and_set_null((void **) &p->title);
        free_and_set_null((void **) &p->cover_uri);
        player_emit_event(p, PLAYER_ACTIVATED);
    } else {
        player_emit_event(p, PLAYER_ACTIVATED);
    }
    player_emit_event(p, PLAYER_PLAYBACK_STOPPED);
}

char *player_get_name(player *p) {
    return p->name;
}

int player_is_active(player *p) {
    return p->active;
}

int player_active_changed(player *p) {
    if (p->active != p->previous_active) {
        p->previous_active = p->active;
        return 1;
    }
    return 0;
}

char *player_get_icon(player *p) {
    return p->icon;
}

char *player_get_label(player *p) {
    return p->label;
}

void player_set_label(player *p, char *label) {
    free_and_set_null((void **) &p->label);
    p->label = label;
}

int player_do_check(player *p) {
    return check_time_interval(p->check_interval);
}

void player_set_album(player *p, char *album) {
    if (!my_strcmp(p->album, album)) {
        return;
    }
    free_and_set_null((void **) &p->album);
    p->album = my_strdup(album);
    player_emit_event(p, PLAYER_METADATA_CHANGED);
}

char *player_get_album(player *p) {
    return p->album;
}

void player_set_artist(player *p, char *artist) {
    if (!my_strcmp(p->artist, artist)) {
        return;
    }
    free_and_set_null((void **) &p->artist);
    p->artist = my_strdup(artist);
    player_emit_event(p, PLAYER_METADATA_CHANGED);
}

char *player_get_artist(player *p) {
    return p->artist;
}

void player_set_title(player *p, char *title) {
    if (!my_strcmp(p->title, title)) {
        return;
    }
    free_and_set_null((void **) &p->title);
    p->title = my_strdup(title);
    player_emit_event(p, PLAYER_METADATA_CHANGED);
}

char *player_get_title(player *p) {
    return p->title;
}

int player_set_cover_uri(player *p, char *cover_uri) {
    p->cover_changed = my_strcmp(p->cover_uri, cover_uri) ? 1 : 0;
    if (p->cover_changed) {
        free_and_set_null((void **) &p->cover_uri);
        p->cover_uri = my_strdup(cover_uri);
    }
    return p->cover_changed;
}

char *player_get_cover_uri(player *p) {
    return p->cover_uri;
}

void player_set_cover_image_path(player *p, const char *path) {
    if (!my_strcmp(p->info_bg_image_path, path)) {
        return;
    }
    free_and_set_null((void **) &p->info_bg_image_path);
    if (path) {
        p->info_bg_image_path = my_copystr(path);
    }
    player_emit_event(p, PLAYER_COVER_CHANGED);
}

char *player_get_cover_image_path(player *p) {
    return p->info_bg_image_path;
}

void player_set_playback_status(player *p, player_status status) {
    if (p->playback_status != status) {
        p->playback_status = status;
        player_emit_event(p, status);
    }
}

player_status player_get_playback_status(player *p) {
    return p->playback_status;
}

void player_set_info_bg_path(player *p, const char *path) {
    player_set_cover_image_path(p, path);
}

char *player_get_info_bg_path(player *p) {
    return player_get_cover_image_path(p);
}

int player_thread_running(player *p) {
    return ((__player_internal *) p)->thread_running;
}

void *__player_thread_function(void *data) {
    __player_internal *p_internal = (__player_internal *) data;
    p_internal->thread_running = 1;

    int result = 1;

    if (p_internal->init_function) {
        result = p_internal->init_function();
    }

    struct timespec sleep_ns = {0, 20 * 1000000};

    while (p_internal->thread_running) {
        __player_action *next_action = player_next_action(&p_internal->player);
        while (next_action != NULL && p_internal->thread_running) {
            next_action->action(next_action->data);
            player_action_free(next_action);
            next_action = player_next_action(&p_internal->player);
        }
        if (next_action) {
            player_action_free(next_action);
        }
        if (!result) {
            break;
        }

        if (p_internal->thread_running && p_internal->run_function
            && check_time_interval(p_internal->player.check_interval)) {
            p_internal->run_function();
        }

        nanosleep(&sleep_ns, NULL);
    }

    if (p_internal->cleanup_function) {
        p_internal->cleanup_function();
    }

    return NULL;
}

int player_start(player *p) {
    __player_internal *p_internal = (__player_internal *) p;

    if (p_internal->thread_running) {
        log_warning(AUDIO_CTX, "Player %s already started.\n", p->name);
        return 0;
    }

    p_internal->player_thread = 0;

    int r = pthread_create(&p_internal->player_thread, NULL, __player_thread_function, p_internal);

    if (r) {
        p_internal->thread_running = 0;
        log_error(AUDIO_CTX, "Could not start thread for %s: %d\n", p->name, r);
        return 1;
    }

    return 0;
}

int player_playback_stop(player *p) {
    __player_internal *p_internal = (__player_internal *) p;
    if (p_internal->playback_stop_function) {
        __player_enqueue_action(p, p_internal->playback_stop_function, NULL);
    }
    return 1;
}

int player_playback_start(player *p, void *song) {
    __player_internal *p_internal = (__player_internal *) p;
    if (p_internal->playback_start_function) {
        __player_enqueue_action(p, p_internal->playback_start_function, song);
    }
    return 1;
}

int player_stop(player *p) {
    __player_internal *p_internal = (__player_internal *) p;
    p_internal->thread_running = 0;
    if (p_internal->abort_function) {
        p_internal->abort_function();
    }
    pthread_join(p_internal->player_thread, NULL);
    return 1;
}

player_event *player_next_event(void) {
    __player_event_node *node = NULL;
    player_event *event = NULL;

    pthread_mutex_lock(&__player_event_mutex);
    if (__player_event_head) {
        node = __player_event_head;
        __player_event_head = node->next;
        if (!__player_event_head) {
            __player_event_tail = NULL;
        }
    }
    pthread_mutex_unlock(&__player_event_mutex);

    if (!node) {
        return NULL;
    }

    event = node->event;
    free(node);
    return event;
}

void player_event_free(player_event *event) {
    free(event);
}

void __player_enqueue_action(player *owner, player_action *action_func, void *data) {
    __player_action_node *node = malloc(sizeof(__player_action_node));

    if (!node) {
        log_error(AUDIO_CTX, "Could not allocate player action node\n");
        return;
    }

    node->action = malloc(sizeof(__player_action));
    if (!node->action) {
        log_error(AUDIO_CTX, "Could not allocate player action\n");
        free(node);
        return;
    }

    node->action->owner = owner;
    node->action->action = action_func;
    node->action->data = data;
    node->next = NULL;

    pthread_mutex_lock(&__player_action_mutex);
    if (__player_action_tail) {
        __player_action_tail->next = node;
    } else {
        __player_action_head = node;
    }
    __player_action_tail = node;
    pthread_mutex_unlock(&__player_action_mutex);
}

__player_action *player_next_action(player *owner) {
    __player_action_node *prev = NULL;
    __player_action_node *node = NULL;
    __player_action *action = NULL;

    pthread_mutex_lock(&__player_action_mutex);
    node = __player_action_head;
    while (node && node->action->owner != owner) {
        prev = node;
        node = node->next;
    }

    if (node) {
        if (prev) {
            prev->next = node->next;
        } else {
            __player_action_head = node->next;
        }
        if (__player_action_tail == node) {
            __player_action_tail = prev;
        }
    }
    pthread_mutex_unlock(&__player_action_mutex);

    if (!node) {
        return NULL;
    }

    action = node->action;
    free(node);
    return action;
}

void player_action_free(__player_action *action) {
    free(action);
}
