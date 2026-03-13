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
#include "playlist.h"
#include "../base.h"
#include <stdlib.h>

int playlist_add_song(playlist *p, song *s) {
    p->n_songs++;
    p->songs = realloc(p->songs, (p->n_songs * sizeof(song *)));
    p->songs[p->n_songs - 1] = s;
    return 1;
}

playlist *playlist_new(char *name) {
    playlist *p = malloc(sizeof(playlist));

    p->n_songs = 0;
    p->songs = NULL;

    if (name) {
        p->name = my_copystr(name);
    } else {
        p->name = NULL;
    }
    return p;
}

int playlist_clear(playlist *p) {
    if (p->songs) {
        for (unsigned int i = 0; i < p->n_songs; i++) {
            song *s = p->songs[i];
            song_free(s);
            p->songs[i] = NULL;
        }
        free (p->songs);
        p->songs = NULL;
        p->n_songs = 0;
    }

    return 0;
}

int playlist_dispose(playlist *p) {
    for (unsigned int i = 0; i < p->n_songs; i++) {
        song *s = p->songs[i];
        song_free(s);
        p->songs[i] = NULL;
    }
    if (p->name)
        free((void *)p->name);
    free (p);
    return 0;
}
