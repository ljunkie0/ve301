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
#ifndef SONG_H
#define SONG_H

#include<limits.h>

const static unsigned int unknown_song_id = UINT_MAX;

typedef struct song {
    char *artist;
    char *name;
    char *title;
    const char *url;
    unsigned int id;
} song;

song *song_new(unsigned int id, const char *url, const char *name, const char *title);
void song_free(song *s);

#endif // SONG_H
