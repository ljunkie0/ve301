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
#include "song.h"
#include "../base/util.h"
#include <stdlib.h>

#define __SONG_MAX_TITLE_LENGTH 20
#define __SONG_MAX_URL_LENGTH 2000

static char *unknown_song_name = "Unknown";
static char *unknown_song_url = "Unknown";

song *song_new(unsigned int id, const char *url, const char *name, const char *title) {
    song *s = malloc(sizeof(song));
    s->id = id;
    if (url) {
        s->url = my_copynstr(url, __SONG_MAX_URL_LENGTH);
    } else {
        s->url = unknown_song_url;
    }
    if (name) {
        s->name = my_copynstr(name, __SONG_MAX_TITLE_LENGTH);
    } else {
        s->name = unknown_song_name;
    }
    if (title) {
        s->title = my_copynstr(title, __SONG_MAX_TITLE_LENGTH);
    } else {
        s->title = unknown_song_name;
    }
    return s;
}

void song_free(song *s) {
    if (s) {
        if (s->name && s->name != unknown_song_name) {
            free (s->name);
        }
        if (s->title && s->title != unknown_song_name) {
            free (s->title);
        }
        if (s->url && s->url != unknown_song_url) {
            free((char *)s->url);
        }
        free (s);
    }
}
