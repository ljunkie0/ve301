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

#ifndef PLAYER_H
#define PLAYER_H
#include "../base/base.h"

typedef int update_player_func();

typedef enum {
    PLAYER_PLAYBACK_UNKNOWN = 0,
    PLAYER_PLAYBACK_PLAYING,
    PLAYER_PLAYBACK_PAUSED,
    PLAYER_PLAYBACK_STOPPED,
    PLAYER_ACTIVATED = 100,
    PLAYER_METADATA_CHANGED,
    PLAYER_COVER_CHANGED,
    PLAYER_VOLUME_CHANGED
} player_status;

typedef void player_action(void *data);
typedef int player_init_function();
typedef int player_run_function();
typedef int player_cleanup_function();
typedef int player_abort_function();

typedef struct player {
    char *name;
    char *icon;
    char *label;
    time_check_interval *check_interval;
    int active;
    int previous_active;
    player_status playback_status;
    char *title;
    char *artist;
    char *album;
    char *cover_uri;
    int cover_changed;
    char *info_bg_image_path;
    long song_id;
} player;

typedef struct player_event {
    player *player;
    player_status status;
} player_event;

player *player_new(const char *name,
                   const char *icon,
                   const char *label,
                   int check_millis,
                   player_init_function *init_function,
                   player_run_function *run_function,
                   player_cleanup_function *cleanup_function,
                   player_abort_function *abort_function,
                   player_action *playback_start_function,
                   player_action *playback_stop_function,
                   player_action *volume_set_function);
char *player_get_name(player *p);
void player_free(player *p);
int player_is_active(player *p);
int player_active_changed(player *p);
char *player_get_icon(player *p);
char *player_get_label(player *p);
char *player_get_album(player *p);
char *player_get_artist(player *p);
char *player_get_title(player *p);
char *player_get_cover_uri(player *p);
char *player_get_cover_image_path(player *p);
char *player_get_info_bg_path(player *p);
long player_get_song_id(player *p);
void player_set_label(player *p, char *label);
player_status player_get_playback_status(player *p);
int player_do_check(player *p);
int player_start(player *p);
int player_stop(player *p);
void player_emit_event(player *p, player_status status);
player_event *player_next_event(void);
void player_event_free(player_event *event);

int player_playback_stop(player *p);
int player_playback_start(player *p, void *song);
int player_volume_set(player *p, int volume);

#endif
