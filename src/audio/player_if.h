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

#ifndef PLAYER_IF_H
#define PLAYER_IF_H

#include "player.h"
void player_set_active(player *p, int active);
void player_set_album(player *p, char *album);
void player_set_artist(player *p, char *artist);
void player_set_title(player *p, char *title);
int player_set_cover_uri(player *p, char *cover_uri);
void player_set_info_bg_path(player *p, const char *path);
void player_set_song_id(player *p, long song_id);
void player_set_cover_image_path(player *p, const char *path);
void player_set_playback_status(player *p, player_status status);
int player_thread_running(player *p);

#endif // PLAYER_IF_H
