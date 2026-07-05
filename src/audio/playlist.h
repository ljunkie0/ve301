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
playlist *playlist_new(const char *name);
playlist *playlist_clone(playlist *p);
int playlist_clear(playlist *p);
void playlist_free(playlist *p);

#endif // PLAYLIST_H
