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
#include "../base/util.h"
#include <stdlib.h>
#include <string.h>

#define MAX_COVER_URL_LENGTH 512

player *player_new(const char *name,
                   const char *icon,
                   const char *label,
                   int seconds_to_check,
                   update_player_func *update) {
    player *p = malloc(sizeof(player));
    p->name = my_copystr(name);
    p->active = 0;
    p->previous_active = 0;
    p->playback_status = PLAYER_PLAYBACK_UNKNOWN;
    p->icon = my_copystr(icon);
    p->label = my_copystr(label);
    p->album = NULL;
    p->artist = NULL;
    p->title = NULL;
    p->cover_uri = NULL;
    p->info_bg_image_path = NULL;
    p->check_interval = time_check_interval_new(seconds_to_check);
    p->update = update;
    p->cover_changed = 0;
    return p;
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
    }
    free (p);
}

void player_set_active(player *p, int active) {
    p->active = active;
    if (!active) {
        p->playback_status = PLAYER_PLAYBACK_STOPPED;
        free_and_set_null((void **) &p->album);
        free_and_set_null((void **) &p->artist);
        free_and_set_null((void **) &p->title);
        free_and_set_null((void **) &p->cover_uri);
    }
}

char *player_get_name(player *p) {
    return p->name;
}

int player_update(player *p) {
    if (p->update) {
        return p->update();
    }
    return 1;
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
    free_and_set_null((void **) &p->album);
    p->album = my_strdup(album);
}

char *player_get_album(player *p) {
    return p->album;
}

void player_set_artist(player *p, char *artist) {
    free_and_set_null((void **) &p->artist);
    p->artist = my_strdup(artist);
}

char *player_get_artist(player *p) {
    return p->artist;
}

void player_set_title(player *p, char *title) {
    free_and_set_null((void **) &p->title);
    p->title = my_strdup(title);
}

char *player_get_title(player *p) {
    return p->title;
}

int player_set_cover_uri(player *p, char *cover_uri) {
    if ((!p->cover_uri && cover_uri) || (p->cover_uri && !cover_uri)
        || strncmp(cover_uri, p->cover_uri, MAX_COVER_URL_LENGTH)) {
        free_and_set_null((void **) &p->cover_uri);
        p->cover_uri = my_strdup(cover_uri);
        p->cover_changed = 1;
    }
    return p->cover_changed;
}

char *player_get_cover_uri(player *p) {
    return p->cover_uri;
}

void player_set_cover_image_path(player *p, const char *path) {
    free_and_set_null((void **) &p->info_bg_image_path);
    if (path) {
        p->info_bg_image_path = my_copystr(path);
    }
}

char *player_get_cover_image_path(player *p) {
    return p->info_bg_image_path;
}

void player_set_playback_status(player *p, player_playback_status status) {
    p->playback_status = status;
}

player_playback_status player_get_playback_status(player *p) {
    return p->playback_status;
}

void player_set_info_bg_path(player *p, const char *path) {
    player_set_cover_image_path(p, path);
}

char *player_get_info_bg_path(player *p) {
    return player_get_cover_image_path(p);
}
