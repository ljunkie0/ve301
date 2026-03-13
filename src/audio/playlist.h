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
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "song.h"

typedef struct playlist {
    const char *name;
    song **songs;
    unsigned int n_songs;
    unsigned int disposed;
} playlist;

int playlist_add_song(playlist *p, song *s);
playlist *playlist_new(char *name);
int playlist_clear(playlist *p);
int playlist_dispose(playlist *p);

#endif // PLAYLIST_H
