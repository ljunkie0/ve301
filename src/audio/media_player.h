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

#ifndef MPD_PLAYER_H
#define MPD_PLAYER_H

#include "player.h"
#include "playlist.h"

player *media_player_new(const char *name, char *icon, const char *label, const long check_millis);
playlist *media_player_get_internet_radios();
int media_player_get_volume();
void media_player_set_volume(int volume);

#endif // MPD_PLAYER_H
