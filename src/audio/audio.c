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
#include<mpd/client.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include"../base.h"
#include"audio.h"

#define MAX_TITLE_LENGTH 20
#define MAX_URL_LENGTH 2000

#ifdef RASPBERRY
#define MPD_HOST "localhost"
#else
#define MPD_HOST "ve301"
#endif
#define MPD_PORT 6600

static struct mpd_connection *mpd_conn;

#define RADIO_PLAYLIST "Radio"

const static unsigned int unknown_song_id = UINT_MAX;
static char *unknown_song_name = "Unknown";
static char *unknown_song_url = "Unknown";

static int *station_id;
static unsigned int n_station_id;
static const int initial_no_of_stations = 20;

static int n_playlists = 0;
static playlist *playlists = 0;

static playlist *internet_radios = NULL;
static song *current_song;

int printplaylists() {
    if (playlists) {
        int p;
        for (p = 0; p < n_playlists; p++) {
            playlist pl = playlists[p];
            log_info(AUDIO_CTX, "Playlist: %s\n", pl.name);
            unsigned int s;
            for (s = 0; s < pl.n_songs; s++) {
                song *so = pl.songs[s];
                log_info(AUDIO_CTX, "\tSong: %s\n", so->title);
            }
        }
    }
    return 1;
}

int init_audio() {
    log_config(AUDIO_CTX, "Audio init\n");

    if (mpd_conn != NULL) {
        log_config(AUDIO_CTX, "init_mpd: Checking existing connection...");
        if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
            log_warning(AUDIO_CTX, "ERROR\n");
            log_warning(AUDIO_CTX, "Error on server: %s\n", mpd_connection_get_error_message(mpd_conn));
            log_warning(AUDIO_CTX, "Trying to reconnect\n");
            mpd_connection_clear_error(mpd_conn);
            mpd_connection_free(mpd_conn);
            mpd_conn = NULL;
        } else {
            log_config(AUDIO_CTX, "OK\n");
        }
    }

    if (mpd_conn == NULL) {
        char *mpd_host = get_config_value("mpd_host",MPD_HOST);
        unsigned int mpd_port = (unsigned int) get_config_value_int("mpd_port",MPD_PORT);
        log_info(AUDIO_CTX, "init_mpd: No connection. Recreating one with time out 30sec.\n");
        mpd_conn = mpd_connection_new(mpd_host, mpd_port, 30000);

        if (mpd_conn == NULL) {
            log_error(AUDIO_CTX, "Can't connect to mpd running on host %s, port %d\n", mpd_host, mpd_port);
            return 0;
        } else {
            enum mpd_error err = mpd_connection_get_error(mpd_conn);
            if (err != MPD_ERROR_SUCCESS) {
                log_error(AUDIO_CTX, "Can't connect to mpd: %s\n", mpd_connection_get_error_message(mpd_conn));
                mpd_conn = NULL;
                return 0;
            }
            log_info(AUDIO_CTX, "Successfully connected to mpd.\n");
            /*mpd_run_clear(mpd_conn);*/
        }

        log_info(AUDIO_CTX, "Running update\n");
        if (!mpd_run_update(mpd_conn, NULL)) {
            log_error(AUDIO_CTX, "Can't run update: %s\n", mpd_connection_get_error_message(mpd_conn));
            mpd_connection_clear_error(mpd_conn);
        }

        n_station_id = 0;
        station_id = malloc(20 * sizeof(int));
        current_song = malloc(sizeof(song));
        current_song->id = 0;
        current_song->name = my_copynstr("Unknown", 7);
        current_song->title = my_copynstr("Unknown", 7);
        current_song->url = unknown_song_url;
    }

    return 1;

}

song *new_song(unsigned int id, const char *url, const char *name, const char *title) {
    song *s = malloc(sizeof(song));
    s->id = id;
    s->url = my_copynstr(url,MAX_URL_LENGTH);
    s->name = my_copynstr(name,MAX_TITLE_LENGTH);
    s->title = my_copynstr(title,MAX_TITLE_LENGTH);
    return s;
}

void song_free(song *s) {
    if (s) {
        if (s->name && s->name != unknown_song_name) {
            free(s->name);
        }
        if (s->title) {
            free(s->title);
        }
        if (s->url && s->url != unknown_song_url) {
            free((char *) s->url);
        }
        free(s);
    }
}

int add_station(const char *url) {
    int id;
    if (init_audio()) {
        id = mpd_run_add_id(mpd_conn, url);

        if ((++n_station_id) > initial_no_of_stations) {
            station_id = realloc(station_id, sizeof(int) * n_station_id);
        }
        station_id[n_station_id - 1] = id;
        if (!(id > 0)) {
            log_error(AUDIO_CTX, "Failed to add %s to playlist.\n", url);
        } else {
            log_info(AUDIO_CTX, "Stream id: %d\n", id);
        }
        return id;
    }

    return -1;

}

int add_song(song *s) {
    if (init_audio()) {
        int id = mpd_run_add_id(mpd_conn, s->url);
        if (id < 0) {
            log_error(AUDIO_CTX, "Failed to add path %s to queue: %s\n", s->url, mpd_connection_get_error_message(mpd_conn));
            mpd_response_finish(mpd_conn);
            return 0;
        }
        s->id = (unsigned int) id;
        return 1;
    } else {
        return 0;
    }
}

int play_by_id(song *s) {
    if (!mpd_run_play_id(mpd_conn, s->id)) {
        log_error(AUDIO_CTX, "Failed to play title: %s\n", mpd_connection_get_error_message(mpd_conn));
        mpd_response_finish(mpd_conn);
        return 0;
    }
    return 1;
}

int play_song(song *s) {
    log_info(AUDIO_CTX, "Trying to play song %s\n", s->title);
    log_info(AUDIO_CTX, "                url %s\n", s->url);
    log_info(AUDIO_CTX, "                 id %d\n", s->id);
    if (init_audio()) {
        if (s->id == unknown_song_id) {
            add_song(s);
            return play_by_id(s);
        }

        if (!mpd_run_play_id(mpd_conn, s->id)) {
            log_error(AUDIO_CTX, "Failed to play title: %s\n", mpd_connection_get_error_message(mpd_conn));
            mpd_response_finish(mpd_conn);
            add_song(s);
            return play_by_id(s);
        }

        mpd_response_finish(mpd_conn);
    }
    return 1;

}

int stop() {
    if (init_audio()) {
        if (!mpd_run_stop(mpd_conn)) {
            log_error(AUDIO_CTX, "Failed to stop playing: %s\n", mpd_connection_get_error_message(mpd_conn));
        }
    }
    return 1;
}

song *get_playing_song() {
    if (init_audio()) {
        struct mpd_song *current_mpd_song;
        struct mpd_status *mpd_stat = mpd_run_status(mpd_conn);
        if (mpd_stat) {
            int playing = (mpd_status_get_state(mpd_stat) == MPD_STATE_PLAY);
            mpd_status_free(mpd_stat);
            if (playing) {
                //			if (current_song->id != song_id) {
                current_mpd_song = mpd_run_current_song(mpd_conn);
                if (current_mpd_song) {
                    if (current_song) {
                        dispose_song(current_song);
                    }

                    unsigned int id = mpd_song_get_id(current_mpd_song);
                    const char *nm = mpd_song_get_tag(current_mpd_song, MPD_TAG_TITLE, 0);
                    if (!nm) {
                        nm = mpd_song_get_tag(current_mpd_song, MPD_TAG_NAME, 0);
                    }
                    if (!nm) {
                        nm = "N/A";
                    }

                    const char *url = mpd_song_get_uri(current_mpd_song);

                    const char *tl = NULL;

                    if (id < unknown_song_id) {
                        unsigned int n;
                        for (n = 0; n < internet_radios->n_songs; n++) {
                            int i = index_of(url,'#');
                            if (i < 0) {
                                i = strlen(url);
                            }
                            if (strncmp(internet_radios->songs[n]->url, url,i) == 0) {
                                tl = internet_radios->songs[n]->title;
                            }
                        }
                    } else {
                        tl = unknown_song_name;
                    }

                    if (!tl) {
                        tl = "  ";
                    }

                    current_song = new_song(id,url,nm,tl);

                    mpd_song_free(current_mpd_song);
                    log_debug(AUDIO_CTX, "Current song: %s - %s\n", current_song->name, current_song->title);
                }
                //			}
                return current_song;
            }
        }
        return 0;
    }
    return 0;
}

int audio_disconnect() {
    if (mpd_conn) {
        /*mpd_run_stop(mpd_conn);
         int n = 0;
         for (n = 0; n < n_station_id; n++) {
         mpd_run_delete_id(mpd_conn, station_id[n]);
         }
         free(station_id);*/
        mpd_connection_free(mpd_conn);
    }
    return 0;
}

int toggle_pause() {

    if (init_audio()) {
        mpd_run_toggle_pause(mpd_conn);
    }
    return 0;
}

int increase_volume() {
    log_config(AUDIO_CTX, "Increasing volume\n");
    if (init_audio()) {
        if (!mpd_send_change_volume(mpd_conn, 2)) {
            log_error(AUDIO_CTX, "Failed to increase volume: %s\n", mpd_connection_get_error_message(mpd_conn));
        }
        mpd_response_finish(mpd_conn);
    }
    return 0;
}

int decrease_volume() {
    log_config(AUDIO_CTX, "Decreasing volume\n");
    if (init_audio()) {
        if (!mpd_send_change_volume(mpd_conn, -2)) {
            log_error(AUDIO_CTX, "Failed to decrease volume: %s\n", mpd_connection_get_error_message(mpd_conn));
        }
        mpd_response_finish(mpd_conn);
    }
    return 0;
}

int get_volume() {
    log_config(AUDIO_CTX, "Get volume\n");
    if (init_audio()) {
        struct mpd_status *status = mpd_run_status(mpd_conn);
        if (!status) {
            log_error(AUDIO_CTX, "Failed to get status: %s\n", mpd_connection_get_error_message(mpd_conn));
            return 0;
        }

        int s = mpd_status_get_volume(status);
        mpd_status_free(status);
        return s;
    }
    return 0;
}

void set_volume(unsigned int volume) {
    log_config(AUDIO_CTX, "Set volume %d\n", volume);
    if (init_audio()) {
        mpd_run_set_volume(mpd_conn,volume);
    }
}

int dispose_song(song *s) {
    song_free(s);
    return 0;
}

int clear_playlist(playlist *p) {
    for (unsigned int i = 0; i < p->n_songs; i++) {
        song *s = p->songs[i];
        dispose_song(s);
        p->songs[i] = 0;
    }
    return 0;
}

int dispose_playlist(playlist *p) {
    for (unsigned int i = 0; i < p->n_songs; i++) {
        song *s = p->songs[i];
        dispose_song(s);
        p->songs[i] = 0;
    }
    if (p->name) free((void *) p->name);
    free(p);
    return 0;
}

#ifdef DBG_MPD
void log_mpd_song(const struct mpd_song *__mpd_song) {
    printf("    MPD_TAG_ALBUM: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ALBUM, 0));
    printf("    MPD_TAG_ALBUM_ARTIST: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ALBUM_ARTIST, 0));
    printf("    MPD_TAG_TITLE: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_TITLE, 0));
    printf("    MPD_TAG_TRACK: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_TRACK, 0));
    printf("    MPD_TAG_NAME: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_NAME, 0));
    printf("    MPD_TAG_GENRE: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_GENRE, 0));
    printf("    MPD_TAG_DATE: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_DATE, 0));
    printf("    MPD_TAG_COMPOSER: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_COMPOSER, 0));
    printf("    MPD_TAG_PERFORMER: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_PERFORMER, 0));
    printf("    MPD_TAG_COMMENT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_COMMENT, 0));
    printf("    MPD_TAG_DISC: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_DISC, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ARTISTID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_ARTISTID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ALBUMID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_ALBUMID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ALBUMARTISTID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_TRACKID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_TRACKID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_RELEASETRACKID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_RELEASETRACKID, 0));
    printf("    MPD_TAG_ORIGINAL_DATE: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ORIGINAL_DATE, 0));
    printf("    MPD_TAG_ARTIST_SORT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ARTIST_SORT, 0));
    printf("    MPD_TAG_ALBUM_ARTIST_SORT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ALBUM_ARTIST_SORT, 0));
    printf("    MPD_TAG_ALBUM_SORT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ALBUM_SORT, 0));
    printf("    MPD_TAG_LABEL: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_LABEL, 0));
    printf("    MPD_TAG_MUSICBRAINZ_WORKID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_WORKID, 0));
    printf("    MPD_TAG_GROUPING: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_GROUPING, 0));
    printf("    MPD_TAG_WORK: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_WORK, 0));
    printf("    MPD_TAG_CONDUCTOR: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_CONDUCTOR, 0));
    printf("    MPD_TAG_COMPOSER_SORT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_COMPOSER_SORT, 0));
    printf("    MPD_TAG_ENSEMBLE: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_ENSEMBLE, 0));
    printf("    MPD_TAG_MOVEMENT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MOVEMENT, 0));
    printf("    MPD_TAG_MOVEMENTNUMBER: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MOVEMENTNUMBER, 0));
    printf("    MPD_TAG_LOCATION: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_LOCATION, 0));
    printf("    MPD_TAG_MOOD: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MOOD, 0));
    printf("    MPD_TAG_TITLE_SORT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_TITLE_SORT, 0));
    printf("    MPD_TAG_MUSICBRAINZ_RELEASEGROUPID: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_MUSICBRAINZ_RELEASEGROUPID, 0));
    printf("    MPD_TAG_COUNT: %s\n",mpd_song_get_tag(__mpd_song,     MPD_TAG_COUNT, 0));
}
#endif

song *add_internet_radio(const struct mpd_song *__mpd_song) {

    const char *name = mpd_song_get_tag(__mpd_song, MPD_TAG_NAME, 0);
    if (!name) name = mpd_song_get_tag(__mpd_song, MPD_TAG_TITLE, 0);
    if (!name) name = mpd_song_get_tag(__mpd_song, MPD_TAG_ARTIST, 0);
    if (!name) name = "N/A";

    const char *path = mpd_song_get_uri(__mpd_song);

    song *s = 0;
    if (path) {
        internet_radios->n_songs++;
        internet_radios->songs = realloc(internet_radios->songs, (internet_radios->n_songs * sizeof(song *)));
        s = new_song(unknown_song_id,path,"N/A",name);
        internet_radios->songs[internet_radios->n_songs - 1] = s;
    }

    return s;

}

playlist *get_internet_radios() {

    if (init_audio()) {

        log_info(AUDIO_CTX, "get_internet_radios: (Re-)creating playlists for internet radios\n");
        if (!internet_radios) {
            internet_radios = malloc(sizeof(playlist));
            internet_radios->name = "Internet Radio";
        } else {
            clear_playlist(internet_radios);
        }

        internet_radios->n_songs = 0;
        internet_radios->songs = 0;

        if (!mpd_send_list_playlists(mpd_conn)) {
            log_error(AUDIO_CTX, "Could not get playlists from server: %s\n", mpd_connection_get_error_message(mpd_conn));
            return internet_radios;
        }

        bool playlist_exists = false;
        struct mpd_playlist *playlist = mpd_recv_playlist(mpd_conn);
        while (playlist) {
            const char *path = mpd_playlist_get_path(playlist);
            if (!strcmp(RADIO_PLAYLIST,path)) {
                playlist_exists = true;
            }
            mpd_playlist_free(playlist);
            playlist = mpd_recv_playlist(mpd_conn);
        }

        if (!playlist_exists) {
            log_info(AUDIO_CTX, "Playlist does not yet exist. Creating it\n");
            mpd_send_save(mpd_conn,RADIO_PLAYLIST);
        }

        if (!mpd_send_list_playlist_meta(mpd_conn,RADIO_PLAYLIST)) {
            log_error(AUDIO_CTX, "Could not list internet-radio playlists: %s\n", mpd_connection_get_error_message(mpd_conn));
            return internet_radios;
        }

        enum mpd_error err = mpd_connection_get_error(mpd_conn);
        if (err != MPD_ERROR_SUCCESS) {
            log_error(AUDIO_CTX, "Could not list internet-radio playlists: %s\n", mpd_connection_get_error_message(mpd_conn));
            return internet_radios;
        }

        struct mpd_song *__mpd_song = mpd_recv_song(mpd_conn);
        while (__mpd_song) {

            song *s = add_internet_radio(__mpd_song);
            if (s) {
                log_info(AUDIO_CTX, "\tSong: %s (%s)\n", s->title, s->url);
            }

            mpd_song_free(__mpd_song);
            __mpd_song = mpd_recv_song(mpd_conn);

        }
    }
    return internet_radios;
}

playlist *get_album_songs(char *album) {
    if (init_audio()) {
        playlist *songs = malloc(sizeof(playlist));
        songs->songs=0;
        songs->n_songs=0;
        songs->name = "Songs";
        if (!mpd_search_db_songs(mpd_conn,1)) {
            log_error(AUDIO_CTX, "Failed get songs for album %s: %s\n", album, mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        if (!mpd_search_add_tag_constraint(mpd_conn,MPD_OPERATOR_DEFAULT,MPD_TAG_ALBUM,album)) {
            log_error(AUDIO_CTX, "Failed get songs for album %s: %s\n", album, mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        if (!mpd_search_commit(mpd_conn)) {
            log_error(AUDIO_CTX, "Failed get songs for album %s: %s\n", album, mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        struct mpd_song *m_song = mpd_recv_song(mpd_conn);
        while (m_song) {
            songs->n_songs++;
            songs->songs = realloc(songs->songs, (songs->n_songs * sizeof(song *)));
            const char *url = mpd_song_get_uri(m_song);
            const char *title = mpd_song_get_tag(m_song,MPD_TAG_TITLE,0);
            song *s = new_song(unknown_song_id,url,"N/A",title);
            songs->songs[songs->n_songs - 1] = s;
            log_info(AUDIO_CTX, "\tSong: %s\n", title);
            mpd_song_free(m_song);
            m_song = mpd_recv_song(mpd_conn);
        }
        return songs;
    }
    return 0;
}

char **get_albums(unsigned int *length) {
    char **albums = 0;
    if (init_audio()) {
        if (!mpd_search_db_tags(mpd_conn,MPD_TAG_ALBUM)) {
            log_error(AUDIO_CTX, "Failed get albums: %s\n", mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        if (!mpd_search_commit(mpd_conn)) {
            log_error(AUDIO_CTX, "Failed get albums: %s\n", mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        unsigned int a = 0;
        struct mpd_pair *pair = mpd_recv_pair_tag(mpd_conn,MPD_TAG_ALBUM);
        while (pair) {
            if (pair->value && strlen(pair->value) > 0) {
                char *album = malloc((strlen(pair->value)+1)*sizeof(char));
                strcpy(album,pair->value);
                log_info(AUDIO_CTX, "Album: %s\n", album);
                albums = realloc(albums,(a+1)*sizeof(char *));
                albums[a++] = album;
            }
            mpd_return_pair(mpd_conn,pair);
            pair = mpd_recv_pair_tag(mpd_conn,MPD_TAG_ALBUM);
        }
        *length = a;
    }
    return albums;
}

struct mpd_connection *get_mpd_connection() {
    if (init_audio()) {
        return mpd_conn;
    }
    return NULL;
}
