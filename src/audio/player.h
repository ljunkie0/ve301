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
#ifndef PLAYER_H
#define PLAYER_H
#include "../base.h"

typedef int update_player_func();

typedef struct player {
    char *name;
    char *icon;
    char *label;
    time_check_interval *check_interval;
    int status;
    int previous_status;
    char *title;
    char *artist;
    char *album;
    char *cover_uri;
    update_player_func *update;
} player;

player *player_new(const char *name,
                   const char *icon,
                   const char *label,
                   int seconds_to_check,
                   update_player_func *update);
char *player_get_name(player *p);
int player_update(player *p);
void player_free(player *p);
int player_get_status(player *p);
void player_set_status(player *p, int status);
int player_status_changed(player *p);
char *player_get_icon(player *p);
char *player_get_label(player *p);
void player_set_label(player *p, char *label);
void player_set_album(player *p, char *album);
char *player_get_album(player *p);
void player_set_artist(player *p, char *artist);
char *player_get_artist(player *p);
void player_set_title(player *p, char *title);
char *player_get_title(player *p);
void player_set_cover_uri(player *p, char *cover_uri);
char *player_get_cover_uri(player *p);
int player_do_check(player *p);
#endif
