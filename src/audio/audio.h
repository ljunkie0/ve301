/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include<inttypes.h>
#include<limits.h>
#include "playlist.h"
#include "song.h"

typedef struct directory {
    char *name;
    char *url;
} directory;

int init_audio(void);
int play_song(song *s);
int audio_disconnect(void);
song *get_playing_song(void);
playlist *get_internet_radios(void);
playlist *get_album_songs(char *album);
playlist *get_artist_songs(char *artist);
playlist *get_artist_album_songs(char *artist, char *album);

char **get_albums(unsigned int *length);
char **get_artists(unsigned int *length);
char **get_artist_albums(char *artist, unsigned int *length);
int stop(void);

#endif /* AUDIO_H_ */
