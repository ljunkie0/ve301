#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "player.h"
#include "player_if.h"
#include "playlist.h"
#include "song.h"
#include <mpd/client.h>
#include <pthread.h>
#include <string.h>

#define MPD_HOST "localhost"
#define MPD_PORT 6600
#define RADIO_PLAYLIST "Radio"

static pthread_mutex_t internet_radios_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t volume_mutex = PTHREAD_MUTEX_INITIALIZER;

struct __mpd_player {
    player *player;
    struct mpd_connection *player_conn;
    int current_song_id;
    time_check_interval *check_internet_radios_interval;
    playlist *internet_radios;
    int current_volume;
} __mpd_player;

static struct mpd_connection *__connect_mpd(const char *context, int timeout_ms);

static void __mpd_update(struct mpd_connection *conn);
static int __mpd_play_song(song *s, struct mpd_connection *conn);
static int __mpd_check_connection(char *context, struct mpd_connection **conn);
static int __mpd_playlist_exists(const char *name, struct mpd_connection *conn);
static song *__mpd_add_song_to_playlist(const struct mpd_song *__mpd_song, const playlist *playlist);
static playlist *__mpd_get_internet_radios(playlist *internet_radios,
                                           int recreate,
                                           struct mpd_connection *conn);
static song *__mpd_find_song_in_playlist(int song_id, playlist *playlist);
static song *__mpd_find_song_in_playlist_by_url(const char *url, playlist *playlist);
static void __mpd_response_finish(struct mpd_connection *conn);

int __mpd_init(void *data) {
    player_set_active(__mpd_player.player, 1);
    return 1;
}

int __mpd_run(void *data) {
    if (__mpd_check_connection("Player", &__mpd_player.player_conn)) {
        __mpd_update(__mpd_player.player_conn);
    }
    return 1;
}

int __mpd_cleanup(void *data) {
    mpd_connection_free(__mpd_player.player_conn);
    time_check_interval_free(__mpd_player.check_internet_radios_interval);
    playlist_free(__mpd_player.internet_radios);
    return 1;
}

void __mpd_playback_start(void *data) {
    song *queued_song = (song *) data;
    if (__mpd_check_connection("Player", &__mpd_player.player_conn)) {
        __mpd_play_song(queued_song, __mpd_player.player_conn);
    }
    song_free(queued_song);
}

void __mpd_playback_stop(void *data) {
    if (__mpd_check_connection("Player", &__mpd_player.player_conn)) {
        if (!mpd_run_stop(__mpd_player.player_conn)) {
            log_error(AUDIO_CTX,
                      "Failed to stop playing: %s\n",
                      mpd_connection_get_error_message(__mpd_player.player_conn));
            if (mpd_connection_get_error(__mpd_player.player_conn) != MPD_ERROR_SUCCESS) {
                mpd_connection_clear_error(__mpd_player.player_conn);
            }
        }
    }
}

void __mpd_volume_set(void *data) {
    int volume = *((int *) data);
    if (__mpd_check_connection("Player", &__mpd_player.player_conn)) {
        pthread_mutex_lock(&volume_mutex);
        if (!mpd_run_set_volume(__mpd_player.player_conn, volume)) {
            log_error(AUDIO_CTX,
                      "Failed to set volume: %s\n",
                      mpd_connection_get_error_message(__mpd_player.player_conn));
            if (mpd_connection_get_error(__mpd_player.player_conn) != MPD_ERROR_SUCCESS) {
                mpd_connection_clear_error(__mpd_player.player_conn);
            }
        } else {
            __mpd_player.current_volume = volume;
        }
        pthread_mutex_unlock(&volume_mutex);
    }
}

player *media_player_new(const char *name, char *icon, const char *label, const long check_millis) {
    player *mpd_player = player_new(name,
                                    icon,
                                    label,
                                    check_millis,
                                    &__mpd_init,
                                    &__mpd_run,
                                    &__mpd_cleanup,
                                    NULL,
                                    &__mpd_playback_start,
                                    &__mpd_playback_stop,
                                    &__mpd_volume_set);

    __mpd_player.player = mpd_player;
    __mpd_player.player_conn = NULL;
    __mpd_player.check_internet_radios_interval = time_check_interval_new(5000);
    __mpd_player.current_song_id = -1;
    __mpd_player.current_volume = -1;

    return mpd_player;
}

playlist *media_player_get_internet_radios() {
    pthread_mutex_lock(&internet_radios_mutex);
    playlist *pl = playlist_clone(__mpd_player.internet_radios);
    pthread_mutex_unlock(&internet_radios_mutex);
    return pl;
}

int media_player_get_volume() {
    pthread_mutex_lock(&internet_radios_mutex);
    int volume = __mpd_player.current_volume;
    pthread_mutex_unlock(&internet_radios_mutex);
    return volume;
}

void media_player_set_volume(int volume) {
    player_volume_set(__mpd_player.player, volume);
}

static playlist *__mpd_get_internet_radios(playlist *internet_radios,
                                           int recreate,
                                           struct mpd_connection *conn) {

    if (!internet_radios) {
        internet_radios = playlist_new("Internet Radio");
    } else if (!recreate) {
        return internet_radios;
    } else {
        playlist_clear(internet_radios);
    }

    log_config(AUDIO_CTX, "get_internet_radios: Recreating playlists for internet radios\n");

    if (!__mpd_playlist_exists(RADIO_PLAYLIST, conn)) {
        log_info(AUDIO_CTX, "Playlist does not yet exist. Creating it\n");
        if (mpd_send_save(conn, RADIO_PLAYLIST)) {
            __mpd_response_finish(conn);
        }
    }

    if (!mpd_send_list_playlist_meta(conn, RADIO_PLAYLIST)) {
        log_error(AUDIO_CTX,
                  "Could not list internet-radio playlists: %s\n",
                  mpd_connection_get_error_message(conn));
        pthread_mutex_unlock(&internet_radios_mutex);
        return NULL;
    }

    struct mpd_song *__mpd_song = mpd_recv_song(conn);
    while (__mpd_song) {
        song *s = __mpd_add_song_to_playlist(__mpd_song, internet_radios);
        if (s) {
            log_info(AUDIO_CTX, "\tSong: %s (%s)\n", s->title, s->url);
        }

        mpd_song_free(__mpd_song);
        __mpd_song = mpd_recv_song(conn);
    }
    __mpd_response_finish(conn);

    return internet_radios;
}

static void __mpd_response_finish(struct mpd_connection *conn) {
    if (!mpd_response_finish(conn)) {
        log_error(AUDIO_CTX,
                  "Failed in waiting for response to finish: %s\n",
                  mpd_connection_get_error_message(conn));
    }
}

static int __mpd_add_song(song *s, struct mpd_connection *conn) {
    int id = mpd_run_add_id(conn, s->url);
    if (id < 0) {
        log_error(AUDIO_CTX,
                  "Failed to add path %s to queue: %s\n",
                  s->url,
                  mpd_connection_get_error_message(conn));
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            mpd_connection_clear_error(conn);
        }
        return 0;
    }
    s->id = (unsigned int) id;
    return 1;
}

static int __mpd_play_by_id(song *s, struct mpd_connection *conn) {
    if (!mpd_run_play_id(conn, s->id)) {
        log_error(AUDIO_CTX, "Failed to play title: %s\n", mpd_connection_get_error_message(conn));
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            mpd_connection_clear_error(conn);
        }
        return 0;
    }
    return 1;
}

static int __mpd_play_song(song *s, struct mpd_connection *conn) {
    log_info(AUDIO_CTX, "Trying to play song %s\n", s->title);
    log_info(AUDIO_CTX, "                url %s\n", s->url);
    log_info(AUDIO_CTX, "                 id %d\n", s->id);
    if (s->id == unknown_song_id) {
        __mpd_add_song(s, conn);
        return __mpd_play_by_id(s, conn);
    }

    if (!mpd_run_play_id(conn, s->id)) {
        log_error(AUDIO_CTX, "Failed to play title: %s\n", mpd_connection_get_error_message(conn));
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            mpd_connection_clear_error(conn);
        }
        __mpd_add_song(s, conn);
        return __mpd_play_by_id(s, conn);
    }
    return 1;
}

static void __mpd_update(struct mpd_connection *conn) {
    struct mpd_song *current_mpd_song = mpd_run_current_song(conn);

    if (current_mpd_song) {
        __mpd_player.current_song_id = mpd_song_get_id(current_mpd_song);

        char *nm = (char *) mpd_song_get_tag(current_mpd_song, MPD_TAG_TITLE, 0);
        if (!nm) {
            nm = (char *) mpd_song_get_tag(current_mpd_song, MPD_TAG_NAME, 0);
        }
        if (nm && (!__mpd_player.player->title || strcmp(__mpd_player.player->title, nm))) {
            player_set_title(__mpd_player.player, nm);
        }

        __mpd_player.internet_radios = __mpd_get_internet_radios(__mpd_player.internet_radios,
                                                                 0,
                                                                 conn);
        song *radio_station = __mpd_find_song_in_playlist(__mpd_player.current_song_id,
                                                          __mpd_player.internet_radios);
        if (!radio_station) {
            radio_station = __mpd_find_song_in_playlist_by_url(mpd_song_get_uri(current_mpd_song),
                                                               __mpd_player.internet_radios);
        }

        char *artist = NULL;
        if (radio_station) {
            artist = radio_station->title ? radio_station->title : radio_station->name;
            if (!artist) {
                artist = radio_station->artist;
            }
        }
        if (!artist) {
            artist = (char *) mpd_song_get_tag(current_mpd_song, MPD_TAG_ARTIST, 0);
        }
        if (artist
            && (!__mpd_player.player->artist || strcmp(__mpd_player.player->artist, artist))) {
            player_set_artist(__mpd_player.player, artist);
        }

        mpd_song_free(current_mpd_song);
    }

    int volume = mpd_run_get_volume(__mpd_player.player_conn);

    if (__mpd_player.current_volume != volume) {
        pthread_mutex_lock(&volume_mutex);
        __mpd_player.current_volume = volume;
        pthread_mutex_unlock(&volume_mutex);
        player_emit_event(__mpd_player.player, PLAYER_VOLUME_CHANGED);
    }

    if (check_time_interval(__mpd_player.check_internet_radios_interval)) {
        pthread_mutex_lock(&internet_radios_mutex);
        __mpd_player.internet_radios = __mpd_get_internet_radios(__mpd_player.internet_radios,
                                                                 1,
                                                                 __mpd_player.player_conn);
        pthread_mutex_unlock(&internet_radios_mutex);
    }
}

static int __mpd_check_connection(char *context, struct mpd_connection **conn) {
    if (!*conn) {
        *conn = __connect_mpd(context, 3000);
    }

    if (*conn) {
        struct mpd_status *mpd_stat = mpd_run_status(*conn);
        if (!mpd_stat) {
            if (mpd_connection_get_error(*conn) != MPD_ERROR_SUCCESS) {
                log_error(AUDIO_CTX,
                          "mpd: Failed to get status: %s\n",
                          mpd_connection_get_error_message(*conn));
                mpd_connection_clear_error(*conn);
            }
            mpd_connection_free(*conn);
            *conn = NULL;
            return 0;
        }
        mpd_status_free(mpd_stat);
    }
    return 1;
}

static struct mpd_connection *__connect_mpd(const char *context, int timeout_ms) {
    char mpd_host[MAX_CONFIG_LINE_LENGTH];

    config_value(mpd_host, "mpd_host", MPD_HOST);
    unsigned int mpd_port = (unsigned int) get_config_value_int("mpd_port", MPD_PORT);

    log_info(AUDIO_CTX,
             "%s: No connection. Recreating one with time out %dsec.\n",
             context,
             timeout_ms / 1000);
    struct mpd_connection *conn = mpd_connection_new(mpd_host, mpd_port, timeout_ms);

    if (!conn) {
        log_error(AUDIO_CTX,
                  "Can't connect to mpd running on host %s, port %d\n",
                  mpd_host,
                  mpd_port);
    } else {
        enum mpd_error err = mpd_connection_get_error(conn);
        if (err != MPD_ERROR_SUCCESS) {
            log_error(AUDIO_CTX,
                      "Can't connect to mpd: %s\n",
                      mpd_connection_get_error_message(conn));
            mpd_connection_free(conn);
            conn = NULL;
        } else {
            log_info(AUDIO_CTX, "Successfully connected to mpd for \"%s\"\n", context);
        }
    }

    return conn;
}

static int __mpd_playlist_exists(const char *name, struct mpd_connection *conn) {
	int playlist_exists = 0;
    if (!mpd_send_list_playlists(conn)) {
        log_error(AUDIO_CTX,
                  "Could not get playlists from server: %s\n",
                  mpd_connection_get_error_message(conn));
        return false;
    }

    struct mpd_playlist *playlist = mpd_recv_playlist(conn);
    while (playlist) {
        const char *path = mpd_playlist_get_path(playlist);
        if (!strcmp(name, path)) {
            playlist_exists = true;
        }
        mpd_playlist_free(playlist);
        playlist = mpd_recv_playlist(conn);
    }
    __mpd_response_finish(conn);
    return playlist_exists;
}

static song *__mpd_add_song_to_playlist(const struct mpd_song *__mpd_song, const playlist *pl) {
    const char *name = mpd_song_get_tag(__mpd_song, MPD_TAG_NAME, 0);
    if (!name)
        name = mpd_song_get_tag(__mpd_song, MPD_TAG_TITLE, 0);
    if (!name)
        name = mpd_song_get_tag(__mpd_song, MPD_TAG_ARTIST, 0);

    const char *path = mpd_song_get_uri(__mpd_song);

    song *s = 0;
    if (path) {
        playlist_add_song((playlist *) pl, song_new(unknown_song_id, path, NULL, name));
    }

    return s;
}

static song *__mpd_find_song_in_playlist(int song_id, playlist *playlist) {
    if (!playlist) {
        return NULL;
    }
    for (int s = 0; s < playlist->n_songs; s++) {
        song *song = playlist->songs[s];
        if (song->id == song_id) {
            return song;
        }
    }
    return NULL;
}

static song *__mpd_find_song_in_playlist_by_url(const char *url, playlist *playlist) {
    if (!url || !playlist) {
        return NULL;
    }
    for (int s = 0; s < playlist->n_songs; s++) {
        song *song = playlist->songs[s];
        if (song->url && !strcmp(song->url, url)) {
            return song;
        }
    }
    return NULL;
}
