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
#include "audio.h"
#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include <mpd/client.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TITLE_LENGTH 20
#define MAX_URL_LENGTH 2000

#define MPD_HOST "localhost"
#define MPD_PORT 6600

static struct mpd_connection *mpd_conn;

#define RADIO_PLAYLIST "Radio"

static playlist *internet_radios = NULL;
static song *current_song;

static pthread_t __audio_thread = 0;
int __audio_thread_running = 0;

void *__audio_connect_thread_func(void *p) {
    __audio_thread_running = 1;
    char mpd_host[MAX_CONFIG_LINE_LENGTH];

    config_value(mpd_host, "mpd_host", MPD_HOST);
    unsigned int mpd_port = (unsigned int) get_config_value_int("mpd_port", MPD_PORT);

    log_info(AUDIO_CTX, "init_mpd: No connection. Recreating one with time out 3sec.\n");
    mpd_conn = mpd_connection_new(mpd_host, mpd_port, 3000);

    if (!mpd_conn) {
        log_error(AUDIO_CTX,
                  "Can't connect to mpd running on host %s, port %d\n",
                  mpd_host,
                  mpd_port);
    } else {
        enum mpd_error err = mpd_connection_get_error(mpd_conn);
        if (err != MPD_ERROR_SUCCESS) {
            log_error(AUDIO_CTX,
                      "Can't connect to mpd: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
            mpd_connection_free(mpd_conn);
            mpd_conn = NULL;
        } else {
            log_info(AUDIO_CTX, "Successfully connected to mpd.\n");
        }
    }

    if (mpd_conn) {
        log_info(AUDIO_CTX, "Running update\n");
        if (!mpd_run_update(mpd_conn, NULL)) {
            log_error(AUDIO_CTX,
                      "Can't run update: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
            mpd_connection_clear_error(mpd_conn);
        }

        current_song = song_new(0, NULL, NULL, NULL);
    }

    __audio_thread_running = 0;

    return 0;
}

int init_audio() {
    log_debug(AUDIO_CTX, "Audio init\n");

    if (mpd_conn != NULL) {
        log_config(AUDIO_CTX, "init_mpd: Checking existing connection...");
        if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
            log_warning(AUDIO_CTX, "ERROR\n");
            log_warning(AUDIO_CTX, "Error on server: %s\n",
                        mpd_connection_get_error_message(mpd_conn));
            log_warning(AUDIO_CTX, "Trying to reconnect\n");
            mpd_connection_clear_error(mpd_conn);
            mpd_connection_free(mpd_conn);
            mpd_conn = NULL;
        } else {
            log_config(AUDIO_CTX, "OK\n");
        }
    }

    if (mpd_conn == NULL) {
        if (!__audio_thread_running) {
            int r = pthread_create(&__audio_thread, NULL, __audio_connect_thread_func, NULL);
            if (r) {
                __audio_thread_running = 0;
                log_error(AUDIO_CTX, "Could not start audio thread: %d\n", r);
            }
        }

        return 0;
    }

    return 1;
}

int add_song(song *s) {
    if (init_audio()) {
        int id = mpd_run_add_id(mpd_conn, s->url);
        if (id < 0) {
            log_error(AUDIO_CTX, "Failed to add path %s to queue: %s\n", s->url,
                      mpd_connection_get_error_message(mpd_conn));
            mpd_response_finish(mpd_conn);
            return 0;
        }
        s->id = (unsigned int)id;
        return 1;
    } else {
        return 0;
    }
}

int play_by_id(song *s) {
    if (!mpd_run_play_id(mpd_conn, s->id)) {
        log_error(AUDIO_CTX, "Failed to play title: %s\n",
                  mpd_connection_get_error_message(mpd_conn));
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
            log_error(AUDIO_CTX, "Failed to play title: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
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
            log_error(AUDIO_CTX, "Failed to stop playing: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
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
                current_mpd_song = mpd_run_current_song(mpd_conn);
                if (current_mpd_song) {
                    if (current_song) {
                        song_free(current_song);
                    }

                    unsigned int id = mpd_song_get_id(current_mpd_song);
                    const char *nm = mpd_song_get_tag(current_mpd_song, MPD_TAG_TITLE, 0);
                    if (!nm) {
                        nm = mpd_song_get_tag(current_mpd_song, MPD_TAG_NAME, 0);
                    }

                    const char *url = mpd_song_get_uri(current_mpd_song);
                    const char *tl = NULL;

                    if (id < unknown_song_id) {
                        unsigned int n;
                        if (!internet_radios) {
                            get_internet_radios();
                        }
                        for (n = 0; n < internet_radios->n_songs; n++) {
                            if (strncmp(internet_radios->songs[n]->url, url, strlen(url)) == 0) {
                                tl = internet_radios->songs[n]->title;
                            }
                        }
                    }

                    current_song = song_new(id, url, nm, tl);

                    mpd_song_free(current_mpd_song);
                    log_debug(AUDIO_CTX, "Current song: %s - %s\n", current_song->name,
                              current_song->title);
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
        mpd_connection_free(mpd_conn);
    }
    song_free(current_song);
    return 0;
}

#ifdef DBG_MPD
void log_mpd_song(const struct mpd_song *__mpd_song) {
    printf("    MPD_TAG_ALBUM: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ALBUM, 0));
    printf("    MPD_TAG_ALBUM_ARTIST: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ALBUM_ARTIST, 0));
    printf("    MPD_TAG_TITLE: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_TITLE, 0));
    printf("    MPD_TAG_TRACK: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_TRACK, 0));
    printf("    MPD_TAG_NAME: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_NAME, 0));
    printf("    MPD_TAG_GENRE: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_GENRE, 0));
    printf("    MPD_TAG_DATE: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_DATE, 0));
    printf("    MPD_TAG_COMPOSER: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_COMPOSER, 0));
    printf("    MPD_TAG_PERFORMER: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_PERFORMER, 0));
    printf("    MPD_TAG_COMMENT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_COMMENT, 0));
    printf("    MPD_TAG_DISC: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_DISC, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ARTISTID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ALBUMID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_ALBUMARTISTID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_TRACKID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_TRACKID, 0));
    printf("    MPD_TAG_MUSICBRAINZ_RELEASETRACKID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_RELEASETRACKID, 0));
    printf("    MPD_TAG_ORIGINAL_DATE: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ORIGINAL_DATE, 0));
    printf("    MPD_TAG_ARTIST_SORT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ARTIST_SORT, 0));
    printf("    MPD_TAG_ALBUM_ARTIST_SORT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ALBUM_ARTIST_SORT, 0));
    printf("    MPD_TAG_ALBUM_SORT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ALBUM_SORT, 0));
    printf("    MPD_TAG_LABEL: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_LABEL, 0));
    printf("    MPD_TAG_MUSICBRAINZ_WORKID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_WORKID, 0));
    printf("    MPD_TAG_GROUPING: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_GROUPING, 0));
    printf("    MPD_TAG_WORK: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_WORK, 0));
    printf("    MPD_TAG_CONDUCTOR: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_CONDUCTOR, 0));
    printf("    MPD_TAG_COMPOSER_SORT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_COMPOSER_SORT, 0));
    printf("    MPD_TAG_ENSEMBLE: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_ENSEMBLE, 0));
    printf("    MPD_TAG_MOVEMENT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MOVEMENT, 0));
    printf("    MPD_TAG_MOVEMENTNUMBER: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MOVEMENTNUMBER, 0));
    printf("    MPD_TAG_LOCATION: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_LOCATION, 0));
    printf("    MPD_TAG_MOOD: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MOOD, 0));
    printf("    MPD_TAG_TITLE_SORT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_TITLE_SORT, 0));
    printf("    MPD_TAG_MUSICBRAINZ_RELEASEGROUPID: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_MUSICBRAINZ_RELEASEGROUPID, 0));
    printf("    MPD_TAG_COUNT: %s\n",
           mpd_song_get_tag(__mpd_song, MPD_TAG_COUNT, 0));
}
#endif

song *add_internet_radio(const struct mpd_song *__mpd_song) {

    const char *name = mpd_song_get_tag(__mpd_song, MPD_TAG_NAME, 0);
    if (!name)
        name = mpd_song_get_tag(__mpd_song, MPD_TAG_TITLE, 0);
    if (!name)
        name = mpd_song_get_tag(__mpd_song, MPD_TAG_ARTIST, 0);

    const char *path = mpd_song_get_uri(__mpd_song);

    song *s = 0;
    if (path) {
        playlist_add_song(internet_radios, song_new(unknown_song_id, path, NULL, name));
    }

    return s;
}

bool playlist_exists(const char *name) {
    bool playlist_exists = false;
    if (init_audio()) {

        if (!mpd_send_list_playlists(mpd_conn)) {
            log_error(AUDIO_CTX, "Could not get playlists from server: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
            return false;
        }

        struct mpd_playlist *playlist = mpd_recv_playlist(mpd_conn);
        while (playlist) {
            const char *path = mpd_playlist_get_path(playlist);
            if (!strcmp(name, path)) {
                playlist_exists = true;
            }
            mpd_playlist_free(playlist);
            playlist = mpd_recv_playlist(mpd_conn);
        }
    }
    return playlist_exists;
}

playlist *get_internet_radios() {

    if (init_audio()) {

        log_config(
            AUDIO_CTX,
            "get_internet_radios: (Re-)creating playlists for internet radios\n");
        if (!internet_radios) {
            internet_radios = playlist_new("Internet Radio");
        } else {
            playlist_clear(internet_radios);
        }

        if (!playlist_exists(RADIO_PLAYLIST)) {
            log_info(AUDIO_CTX, "Playlist does not yet exist. Creating it\n");
            mpd_send_save(mpd_conn, RADIO_PLAYLIST);
        }

        if (!mpd_send_list_playlist_meta(mpd_conn, RADIO_PLAYLIST)) {
            log_error(AUDIO_CTX, "Could not list internet-radio playlists: %s\n",
                      mpd_connection_get_error_message(mpd_conn));
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

playlist *get_songs(enum mpd_tag_type matchTag1, char *match1, enum mpd_tag_type matchTag2, char *match2) {
    if (init_audio()) {
        playlist *songs = playlist_new(match1);
        if (!mpd_search_db_songs(mpd_conn, 1)) {
            log_error(AUDIO_CTX, "Failed get songs for %s %s and %s %s: %s\n", mpd_tag_name(matchTag1), match1, mpd_tag_name(matchTag2), match2,
                      mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        if (match1) {
            if (!mpd_search_add_tag_constraint(mpd_conn, MPD_OPERATOR_DEFAULT,
                                               matchTag1, match1)) {
                log_error(AUDIO_CTX, "Failed get songs for %s %s and %s %s: %s\n", mpd_tag_name(matchTag1), match1, mpd_tag_name(matchTag2), match2,
                          mpd_connection_get_error_message(mpd_conn));
                return 0;
            }
        }
        if (match2) {
            if (!mpd_search_add_tag_constraint(mpd_conn, MPD_OPERATOR_DEFAULT,
                                               matchTag2, match2)) {
                log_error(AUDIO_CTX, "Failed get songs for %s %s and %s %s: %s\n", mpd_tag_name(matchTag1), match1, mpd_tag_name(matchTag2), match2,
                          mpd_connection_get_error_message(mpd_conn));
                return 0;
            }
        }
        if (!mpd_search_commit(mpd_conn)) {
            log_error(AUDIO_CTX, "Failed get songs for %s %s and %s %s: %s\n", mpd_tag_name(matchTag1), match1, mpd_tag_name(matchTag2), match2,
                      mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        struct mpd_song *m_song = mpd_recv_song(mpd_conn);
        while (m_song) {
            const char *url = mpd_song_get_uri(m_song);
            const char *title = mpd_song_get_tag(m_song, MPD_TAG_TITLE, 0);
            playlist_add_song(songs, song_new(unknown_song_id, url, "N/A", title));
            log_info(AUDIO_CTX, "\tSong: %s\n", title);
            mpd_song_free(m_song);
            m_song = mpd_recv_song(mpd_conn);
        }
        return songs;
    }
    return 0;
}

playlist *get_album_songs(char *album) {
    return get_songs(MPD_TAG_ALBUM, album, MPD_TAG_UNKNOWN, 0);
}

playlist *get_artist_songs(char *artist) {
    return get_songs(MPD_TAG_ARTIST, artist, MPD_TAG_UNKNOWN, 0);
}

playlist *get_artist_album_songs(char *artist, char *album) {
    return get_songs(MPD_TAG_ARTIST, artist, MPD_TAG_ALBUM, album);
}

char **get_items(enum mpd_tag_type tag, enum mpd_tag_type matchTag1,
                 char *match1, enum mpd_tag_type matchTag2, char *match2,
                 unsigned int *length) {
    char **result = 0;
    if (init_audio()) {
        if (!mpd_search_db_tags(mpd_conn, tag)) {
            log_error(AUDIO_CTX, "Failed get items of type %s: %s\n",
                      mpd_tag_name(tag), mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        if (match1) {
            if (!mpd_search_add_tag_constraint(mpd_conn, MPD_OPERATOR_DEFAULT,
                                               matchTag1, match1)) {
                log_error(AUDIO_CTX, "Failed get items for constraint %s: %s\n",
                          mpd_tag_name(matchTag1),
                          mpd_connection_get_error_message(mpd_conn));
                return 0;
            }
        }
        if (match2) {
            if (!mpd_search_add_tag_constraint(mpd_conn, MPD_OPERATOR_DEFAULT,
                                               matchTag2, match2)) {
                log_error(AUDIO_CTX, "Failed get items for constraint %s: %s\n",
                          mpd_tag_name(matchTag2),
                          mpd_connection_get_error_message(mpd_conn));
                return 0;
            }
        }
        if (!mpd_search_commit(mpd_conn)) {
            log_error(AUDIO_CTX, "Failed get items of type %d: %s\n",
                      mpd_tag_name(tag), mpd_connection_get_error_message(mpd_conn));
            return 0;
        }
        unsigned int a = 0;
        struct mpd_pair *pair = mpd_recv_pair_tag(mpd_conn, tag);
        while (pair) {
            if (pair->value && strlen(pair->value) > 0) {
                char *item = malloc((strlen(pair->value) + 1) * sizeof(char));
                strcpy(item, pair->value);
                log_config(AUDIO_CTX, "Item: %s\n", item);
                result = realloc(result, (a + 1) * sizeof(char *));
                result[a++] = item;
            }
            mpd_return_pair(mpd_conn, pair);
            pair = mpd_recv_pair_tag(mpd_conn, tag);
        }
        *length = a;
    }
    return result;
}

char **get_albums(unsigned int *length) {
    return get_items(MPD_TAG_ALBUM, MPD_TAG_UNKNOWN, 0, MPD_TAG_UNKNOWN, 0,
                     length);
}

char **get_artists(unsigned int *length) {
    return get_items(MPD_TAG_ARTIST, MPD_TAG_UNKNOWN, 0, MPD_TAG_UNKNOWN, 0,
                     length);
}

char **get_artist_albums(char *artist, unsigned int *length) {
    return get_items(MPD_TAG_ALBUM, MPD_TAG_ARTIST, artist, MPD_TAG_UNKNOWN, 0,
                     length);
}
