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

typedef struct song {
    char *name;
    char *title;
    const char *url;
    unsigned int id;
} song;

typedef struct directory {
    char *name;
    char *url;
} directory;

typedef struct playlist {
    const char *name;
    song **songs;
    unsigned int n_songs;
} playlist;

int init_audio(void);
int add_station(const char *url);
int play_station(int station_id);
int play_song(song *s);
int dispose_song(song *s);
int audio_disconnect(void);
song *get_playing_song(void);
playlist *get_internet_radios(void);
playlist *get_album_songs(char *album);
char **get_albums(unsigned int *length);
int increase_volume(void);
int decrease_volume(void);
void set_volume(unsigned int volume);
int get_volume(void);
int toggle_pause(void);
int stop(void);
int list_playlists(void);
int dispose_playlist(playlist *p);
struct mpd_connection *get_mpd_connection(void);
//void **get_directory(char *path);

#endif /* AUDIO_H_ */
