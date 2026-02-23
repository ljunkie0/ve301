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

#include "audio/alsa.h"
#include "audio/audio.h"
#include "audio/player.h"
#include "base.h"
#include "base/config.h"
#include "base/log_contexts.h"
#include "base/logging.h"
#include "base/util.h"
#include "menu/menu_ctrl.h"
#include "theme.h"
#include <math.h>
#include <mpd/client.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef BLUETOOTH
#include "audio/bluetooth.h"
#endif
#ifdef SPOTIFY
#include "audio/spotify.h"
#endif
#include "sdl_util.h"
#include "weather.h"
#include "wifi.h"

#ifdef RASPBERRY
#ifdef SDL1
#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#else
#define DEFAULT_WINDOW_WIDTH 720
#define WINDOW_HEIGHT 576
#endif
#else
#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#endif
#define NO_OF_SCALES 60

#ifdef RASPBERRY
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#else
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#endif
#define DEFAULT_X_OFFSET 0
#define DEFAULT_Y_OFFSET 0
#define DEFAULT_LABEL_RADIUS 100
#define DEFAULT_SCALES_RADIUS_START 120
#define DEFAULT_SCALES_RADIUS_END 200
#define DEFAULT_ANGLE_OFFSET 0.0
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_INFO_FONT_SIZE 24
#define CALLBACK_SECONDS 5
#define CHECK_INTERNET_SECONDS 1
#define INFO_MENU_SECONDS 14
#define INFO_MENU_ITEM_SECONDS 5

#ifdef PERF_TEST
/**
 * Performance test
 */
#define MENU_ACTION_LISTENER menu_action_listener_dbg
#define MENU_CALL_BACK menu_call_back_dbg
#else
#define MENU_ACTION_LISTENER menu_action_listener
#define MENU_CALL_BACK menu_call_back
#endif

typedef struct radio_config {
    char info_bg_image_path[MAX_CONFIG_LINE_LENGTH];
    char font[MAX_CONFIG_LINE_LENGTH];
    char info_font[MAX_CONFIG_LINE_LENGTH];
    int font_size;
    int info_font_size;
    char weather_font[MAX_CONFIG_LINE_LENGTH];
    int weather_font_size;
    int temp_font_size;
    int time_font_size;
    int w;
    int h;
    int y_offset;
    int x_offset;
    double angle_offset;
    int radius_labels;
    int radius_scales_start;
    int radius_scales_end;
    int radio_radius_labels;
    int draw_scales;
    int light_x;
    int light_y;
    int light_radius;
    int light_alpha;
    char light_img[MAX_CONFIG_LINE_LENGTH];
    int light_img_x;
    int light_img_y;
    int warp_speed;
    char mixer_device[MAX_CONFIG_LINE_LENGTH];
    char alsa_mixer_name[MAX_CONFIG_LINE_LENGTH];
    int info_menu_item_seconds;
    char weather_api_key[MAX_CONFIG_LINE_LENGTH];
    char weather_location[MAX_CONFIG_LINE_LENGTH];
    char weather_units[MAX_CONFIG_LINE_LENGTH];
} radio_config;

typedef enum {
    OBJ_TYPE_SONG,
    OBJ_TYPE_PLAYLIST,
    OBJ_TYPE_ALBUM,
    OBJ_TYPE_ARTIST,
    OBJ_TYPE_DIRECTORY,
    OBJ_TYPE_TIME,
    OBJ_TYPE_MIXER
} object_type;

static const char *time_menu_item_format = "%H:%M\n%d. %B";

/**
 * Alsa
 **/
static char *default_mixer = "Master";

/**
 * Radio
 */
#define CHECK_RADIO_SECONDS 1
static player *radio_player = NULL;

#ifdef BLUETOOTH
/**
 * Bluetooth
 */
#define CHECK_BLUETOOTH_SECONDS 1
player *bluetooth_player = NULL;
#endif

#ifdef SPOTIFY
/**
 * Spotify
 */
#define CHECK_SPOTIFY_SECONDS 1
player *spotify_player = NULL;
#endif

/**
 * Internet
 */
static int internet_available;
time_check_interval *check_internet_interval;

static int info_menu_item_seconds = INFO_MENU_ITEM_SECONDS;

#ifndef M_PI
#define M_PI 3.1415926536
#endif

typedef struct station {
    const char *label;
    const char *url;
    int id;
} station;

static radio_theme *default_theme = NULL;
static radio_theme *bluetooth_theme = NULL;
static radio_theme *spotify_theme = NULL;

static struct tm *current_tm_info;

static time_t info_menu_t = 0;
static time_t callback_t = 0;

/**
 *
 * Specific Menus
 *
 **/

/* Radio menu */
static menu *radio_menu;
static menu_item *radio_menu_item;
#define RADIO_MENU_SEGMENTS_PER_ITEM 1
#define RADIO_MENU_NO_OF_LINES 3
#define RADIO_MENU_ITEMS_ON_SCALE_FACTOR 4
static int radio_radius_labels;

static menu *info_menu = NULL;
static menu *nav_menu = NULL;
static menu *lib_menu = NULL;
static menu *album_menu = NULL;
static menu *artist_menu = NULL;
static menu *song_menu = NULL;
static menu *system_menu = NULL;
static menu *current_menu = NULL;
static menu *volume_menu = NULL;
static menu *message_menu = NULL;
static menu *mixer_menu = NULL;
static menu_item *message_menu_item = NULL;
/**
 * Specific Menu Items
 **/
static menu_item *player_item = NULL;
static menu_item *title_item = NULL;
static menu_item *artist_item = NULL;
static menu_item *time_item = NULL;
static menu_item *weather_item = NULL;
static menu_item *temperature_item = NULL;
static menu_item *volume_menu_item = NULL;

static weather wthr;

static menu_ctrl *ctrl = NULL;

void read_radio_config(radio_config *config) {
    config_value_path(config->font, "font", DEFAULT_FONT);
    config_value_path(config->info_font, "info_font", config->font);
    config_value_path(config->info_bg_image_path, "info_bg_image_path", NULL);
    config->font_size = get_config_value_int("font_size", DEFAULT_FONT_SIZE);
    config->info_font_size = get_config_value_int("info_font_size", DEFAULT_INFO_FONT_SIZE);
    config_value_path(config->weather_font, "weather_font", DEFAULT_FONT);
    config->weather_font_size = get_config_value_int("weather_font_size", config->info_font_size);
    config->temp_font_size = get_config_value_int("temperature_font_size", config->info_font_size);
    config->time_font_size = get_config_value_int("time_font_size", config->info_font_size);
    config->w = get_config_value_int("window_width", DEFAULT_WINDOW_WIDTH);
    config->h = get_config_value_int("window_height", 0);
    config->y_offset = get_config_value_int("y_offset", DEFAULT_Y_OFFSET);
    config->x_offset = get_config_value_int("x_offset", DEFAULT_Y_OFFSET);
    config->angle_offset = get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
    config->radius_labels = get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
    config->radius_scales_start = get_config_value_int("radius_scales_start",
                                                       DEFAULT_SCALES_RADIUS_START);
    config->radius_scales_end = get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_END);
    config->draw_scales = get_config_value_int("draw_scales", 1);
    config->light_x = get_config_value_int("light_x", (int) config->w / 2);
    config->light_y = get_config_value_int("light_y", 100);
    config->light_radius = get_config_value_int("light_radius", 300);
    config->light_alpha = get_config_value_int("light_alpha", 0);
    config_value_path(config->light_img, "light_image_path", NULL);
    config->light_img_x = get_config_value_int("light_image_x", 0);
    config->light_img_y = get_config_value_int("light_image_y", 0);
    config->warp_speed = get_config_value_int("warp_speed", 10);
    config_value_path(config->mixer_device, "alsa_mixer_device", "default");
    config->radio_radius_labels = get_config_value_int("radio_radius_labels", config->radius_labels);
    config->info_menu_item_seconds = get_config_value_int("info_menu_item_seconds",
                                                          INFO_MENU_ITEM_SECONDS);
    config_value_path(config->alsa_mixer_name, "alsa_mixer_name", "Master");

    config_value(config->weather_api_key, "weather_api_key", "");
    config_value(config->weather_location, "weather_location", "");
    config_value(config->weather_units, "weather_units", "metric");
}

void vol_label(char *buffer, int volume) {
    if (volume < 0) {
        volume = 0;
    }
    snprintf(buffer, 20, "Volume: %d%%", volume);
}

int item_action_update_directories_menu(menu_event evt, menu *m,
                                        menu_item *item);

void apply_radio_theme(menu_ctrl *ctrl, radio_theme *theme) {

    menu_ctrl_apply_theme(ctrl, theme->menu_theme);

    if (theme->info_bg_image_path) {
        menu_set_bg_image(info_menu, theme->info_bg_image_path);
    }

    if (theme->volume_bg_image_path) {
        menu_set_bg_image(volume_menu, theme->volume_bg_image_path);
    }

    SDL_Color *info_default_clr = NULL, *info_selected_clr = NULL;
    if (theme->info_color != NULL) {
        info_default_clr = html_to_color(theme->info_color);
        info_selected_clr = html_to_color(theme->info_color);
    }

    SDL_Color *info_scale_clr = NULL;
    if (theme->info_scale_color != NULL) {
        info_scale_clr = html_to_color(theme->info_scale_color);
    }

    menu_set_colors(info_menu, info_default_clr, info_selected_clr,
                    info_scale_clr);
    menu_set_colors(volume_menu, info_default_clr, info_selected_clr,
                    info_scale_clr);
    menu_set_colors(message_menu, info_default_clr, info_selected_clr,
                    info_scale_clr);

    free (info_default_clr);
    free (info_selected_clr);
    free (info_scale_clr);
}

void reset_info_menu_timer() {
    time_t timer;
    time(&timer);
    info_menu_t = timer;
    callback_t = timer;
}

void update_radio_menu(menu_ctrl *ctrl) {

    menu_clear(radio_menu);

    playlist *internet_radios = get_internet_radios();
    if (internet_radios != NULL) {

        menu_set_no_items_on_scale(radio_menu, RADIO_MENU_ITEMS_ON_SCALE_FACTOR * menu_ctrl_get_n_o_items_on_scale(ctrl));
        menu_set_radius_labels(radio_menu, radio_radius_labels);
        unsigned int r = 0;
        for (r = 0; r < internet_radios->n_songs; r++) {
            song *s = internet_radios->songs[r];
            menu_item_new(radio_menu, s->title, NULL, s, OBJ_TYPE_SONG, NULL, -1,
                          NULL, NULL, -1);
        }

        menu_item_set_sub_menu(radio_menu_item, radio_menu);

    } else {
        menu_item_new(radio_menu, "No radio", NULL, NULL, 0, NULL, -1, NULL, NULL, -1);

        log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
    }
}

void update_album_menu(char *artist) {
    menu_clear(album_menu);
    unsigned int a;
    char **albums = !artist ? get_albums(&a) : get_artist_albums(artist, &a);
    if (albums != NULL) {
        log_config(MAIN_CTX, "Album successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_config(MAIN_CTX, "Getting album %d\n", albums[r]);
            menu_item *mi = menu_add_sub_menu(album_menu, albums[r], song_menu, NULL);
            menu_item_set_object_type(mi, OBJ_TYPE_ALBUM);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: album list is NULL\n");
        menu_item_new(album_menu, "Could not retrieve albums", NULL, NULL,
                      UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_artist_menu() {
    menu_clear(artist_menu);
    unsigned int a;
    char **artists = get_artists(&a);
    if (artists != NULL) {
        log_config(MAIN_CTX, "Artists successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_config(MAIN_CTX, "Getting artists %d\n", r);
            menu_item *mi = menu_add_sub_menu(artist_menu, artists[r], album_menu, NULL);
            menu_item_set_object_type(mi, OBJ_TYPE_ARTIST);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: artists list is NULL\n");
        menu_item_new(artist_menu, "Could not retrieve artists", NULL, NULL,
                      UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_song_menu(char *album, char *artist) {
    log_config(MAIN_CTX, "Update song menu: album = %s, artist = %s\n", album, artist);
    int id = 0;
    for (id = 0; id <= menu_get_max_id(song_menu); id++) {
        song_free((song *) menu_item_get_object(menu_get_item(song_menu, id)));
    }

    menu_clear(song_menu);

    playlist *album_songs = NULL;
    if (album) {
        if (artist) {
            album_songs = get_artist_album_songs(artist, album);
        } else {
            album_songs = get_album_songs(album);
        }
    } else if (artist) {
        album_songs = get_artist_songs(artist);
    }

    if (album_songs != NULL) {
        unsigned int r = 0;
        for (r = 0; r < album_songs->n_songs; r++) {
            song *s = album_songs->songs[r];
            log_info(MAIN_CTX, "Song: %s\n", s->title);
            menu_item_new((menu *)song_menu, s->title, NULL, s,
                          OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
        }
        playlist_dispose(album_songs);
    } else {
        log_error(MAIN_CTX, "update_song_menu: playlist is NULL\n");
        menu_item_new((menu *)song_menu, "FEHLER", NULL, NULL,
                      UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_time_item(menu_item *time_menu_item) {
    char *buffer = malloc(25 * sizeof(char));

    strftime(buffer, 25, time_menu_item_format, current_tm_info);
    menu_item_update_label(time_menu_item, buffer);
    log_debug(MAIN_CTX, "free (buffer))\n");
    log_debug(MAIN_CTX, "buffer = %p\n", buffer);
    free (buffer);
}

int item_action_update_interface_menu(menu_event evt, menu *m,
                                      menu_item *item) {
    if (evt == DISPOSE) {
        if (menu_item_get_object(item)) {
            network_interface *interface = (network_interface *)menu_item_get_object(item);
            free_network_interface(interface);
        }
    } else if (menu_item_get_sub_menu(item)) {

        menu *sub_menu = menu_item_get_sub_menu(item);

        menu_clear(sub_menu);

        network_interface *interface = (network_interface *)menu_item_get_object(item);
        char *ip_address_label = my_catstr("IP\n", interface->ipaddress);
        menu_item_new(sub_menu, ip_address_label, NULL, NULL,
                      UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        free (ip_address_label);

        log_debug(MAIN_CTX, "Getting result from wifi scan\n");
        struct station_info *wifi_scan_result =
            scan_wifi_station(interface->ifname);
        log_debug(MAIN_CTX, "Result: %p\n", wifi_scan_result);

        if (wifi_scan_result) {
            char *ssid_label = my_catstr("Station\n", wifi_scan_result->ssid);
            log_debug(MAIN_CTX, "%s\n", ssid_label);
            menu_item_new(sub_menu, ssid_label, NULL, NULL, UNKNOWN_OBJECT_TYPE,
                          NULL, -1, NULL, NULL, -1);
            free (ssid_label);

            char signal_chr[100];
            sprintf(signal_chr, "%d dBm", wifi_scan_result->signal_dbm);
            char *strength_label = my_catstr("Strength\n", signal_chr);
            menu_item_new(sub_menu, strength_label, NULL, NULL,
                          UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
            free (strength_label);

            free (wifi_scan_result);
            log_debug(MAIN_CTX, "Done\n");
        }
    }

    return 0;
}

int item_action_update_network_menu(menu_event evt, menu *m, menu_item *item) {

    if (evt == DISPOSE) {
        return 0;
    }

    if (menu_item_get_sub_menu(item)) {

        menu *sub_menu = menu_item_get_sub_menu(item);

        int id = menu_get_max_id(sub_menu);
        while (id >= 0) {
            menu_item *i = menu_get_item(sub_menu, id);
            if (menu_item_get_object(i)) {
                free((void *)menu_item_get_object(menu_get_item(sub_menu, id)));
                menu_item_set_object(i,0);
            }
            id--;
        }
        menu_clear(sub_menu);
    }

    network_interfaces *interfaces = get_network_interfaces();
    if (interfaces) {
        for (int i = 0; i < interfaces->n; i++) {
            network_interface *interface = interfaces->interfaces[i];
            menu *interface_menu = menu_new(menu_get_ctrl(m), 1, NULL, 0, NULL, NULL, 0);
            menu_set_no_items_on_scale(interface_menu,3);
            menu_item *interface_item =
                menu_add_sub_menu(menu_item_get_sub_menu(item), interface->ifname, interface_menu, &item_action_update_interface_menu);
            menu_item_set_object(interface_item, interface);
        }
    }

    return 0;
}

int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    if (log_level_enabled(MAIN_CTX, IR_LOG_LEVEL_CONFIG)) {
        char *label = item_ptr ? menu_item_get_label(item_ptr) : "";
        log_config(MAIN_CTX, "action(%d, %p, %s)\n", evt, m_ptr, label);
    }
    if (evt == ACTIVATE) {
        if (item_ptr) {
            menu_item *item = (menu_item *)item_ptr;
            char *label = menu_item_get_label(item);
            log_config(MAIN_CTX, "Action: %s\n", label);
            if (menu_item_get_sub_menu(item)) {
                menu *sub_menu = menu_item_get_sub_menu(item);
                log_config(MAIN_CTX, "Sub menu: %s\n", label);
                if (item == radio_menu_item) {
                    update_radio_menu(ctrl);
                } else if (sub_menu == album_menu) {
                    if (menu_item_get_object_type(item) == OBJ_TYPE_ARTIST) {
                        update_album_menu(label);
                    } else {
                        update_album_menu(NULL);
                    }
                } else if (sub_menu == artist_menu) {
                    update_artist_menu();
                } else if (sub_menu == song_menu) {
                    if (menu_get_parent(m_ptr) == artist_menu) {
                        update_song_menu(label, menu_get_label(m_ptr));
                    } else {
                        update_song_menu(label, NULL);
                    }
                }
            } else if (menu_item_get_object(item) && menu_item_get_object_type(item) == OBJ_TYPE_SONG) {
                menu_set_active_id(m_ptr,menu_item_get_id(item));
                song *s = (song *)menu_item_get_object(item);
                song *p = get_playing_song();
                if (!p || (p->id != s->id)) {
                    if (!play_song(s)) {
                        log_error(MAIN_CTX, "Could not play song %s", s->title);
                    }
                    song *current_song = get_playing_song();
                    if (current_song) {
                        menu_item_update_label(artist_item, current_song->name);
                    }
                } else if (p) {
                    stop();
                }
            } else if (menu_is_transient(menu_item_get_menu(item))) {
                if (menu_ctrl_get_active(ctrl)) {
                    menu_open(menu_ctrl_get_active(ctrl));
                } else {
                    menu_open(nav_menu);
                }
            }
        } else if (m_ptr && menu_is_transient(m_ptr)) {
            if (menu_ctrl_get_active(ctrl)) {
                menu_open(menu_ctrl_get_active(ctrl));
            } else {
                menu_open(nav_menu);
            }
        }
    } else if (evt == ACTIVATE_1) {
        if (m_ptr == mixer_menu && item_ptr) {
            menu_open_sub_menu(ctrl, item_ptr);
        } else {
            menu_open(mixer_menu);
        }
    } else if (evt == HOLD_1) {
        if (m_ptr == mixer_menu) {
            log_config(MAIN_CTX, "Opening volume menu\n");
            menu_open(volume_menu);
        } else {
            log_config(MAIN_CTX, "Opening mixer menu\n");
            menu_open(mixer_menu);
        }
    } else if (evt == TURN_LEFT_1) {
        if (m_ptr == mixer_menu) {
            menu_turn_left(m_ptr);
        } else if (menu_get_current_item(m_ptr) &&
                   menu_item_get_object_type(menu_get_current_item(m_ptr)) == OBJ_TYPE_MIXER) {
            menu_item *mixer_item = menu_get_current_item(m_ptr);
            char *mixer_name = (char *)menu_item_get_object(mixer_item);
            int vol = alsa_get_volume(mixer_name);
            log_config(MAIN_CTX, "Current volume of mixer %s: %d\n", mixer_name, vol);
            alsa_set_volume(default_mixer, vol - 2);
            log_config(MAIN_CTX, "Decreased volume: %d\n", vol);
            char label[13];
            vol_label(label, vol);
            menu_item_update_label(mixer_item, label);
        } else {
            menu_item_show(volume_menu_item);
            int vol = alsa_get_volume(default_mixer);
            log_config(MAIN_CTX, "Current volume: %d\n", vol);
            alsa_set_volume(default_mixer, vol - 2);
            vol = alsa_get_volume(default_mixer);
            log_config(MAIN_CTX, "Decreased volume: %d\n", vol);
            char label[13];
            vol_label(label, vol);
            menu_item_update_label(volume_menu_item, label);
            menu_item_show(volume_menu_item);
        }
    } else if (evt == TURN_RIGHT_1) {
        if (m_ptr == mixer_menu) {
            menu_turn_right((menu *)m_ptr);
        } else {
            menu_item_show(volume_menu_item);
            int vol = alsa_get_volume(default_mixer);
            alsa_set_volume(default_mixer, vol + 2);
            vol = alsa_get_volume(default_mixer);
            char label[13];
            vol_label(label, vol);
            menu_item_update_label(volume_menu_item, label);
            menu_item_show(volume_menu_item);
        }
    } else if (evt == DISPOSE) {
        const void *obj = menu_item_get_object(item_ptr);
        if (obj && menu_item_get_menu(item_ptr) != radio_menu) {

            switch(menu_item_get_object_type(item_ptr)) {
            case OBJ_TYPE_DIRECTORY:
                log_debug(MAIN_CTX, "free(%p)\n", obj);
                free((void *)obj);
                menu_item_set_object(item_ptr,NULL);
                break;
            case OBJ_TYPE_SONG:
                song_free((song *) obj);
                menu_item_set_object(item_ptr,NULL);
                break;
            case OBJ_TYPE_ALBUM:
            case OBJ_TYPE_ARTIST:
                menu_dispose(menu_item_get_sub_menu(item_ptr));
                break;
            }

        }
    }

    reset_info_menu_timer();

    return 0;
}

void update_menu_item_icon_or_label(menu_item *item, char *icon, char *label) {
    if (icon) {
        menu_item_update_icon(item, icon);
        menu_item_update_label(item, NULL);
    } else {
        menu_item_update_label(item, label);
        menu_item_update_icon(item, NULL);
    }
}

void update_title_artist_menu_item(menu_item *item, char *label, char *icon,
                                   char *label_default) {
    if (label) {
        update_menu_item_icon_or_label(item, NULL, label);
    } else {
        update_menu_item_icon_or_label(item, icon, label_default);
    }
}

int check_player(menu_ctrl *ctrl, player *p, radio_theme *player_theme) {
    if (player_do_check(p)) {
        log_config(MAIN_CTX, "Checking %s...\n", player_get_name(p));
        if (player_update(p)) {
            if (player_get_status(p)) {
                log_config(MAIN_CTX, "%s is connected.\n", player_get_name(p));
                if (player_status_changed(p)) {
                    stop();
                    if (player_theme) {
                        apply_radio_theme(ctrl, player_theme);
                    }
                    update_menu_item_icon_or_label(player_item, player_get_icon(p),
                                                   player_get_label(p));
                }

                update_title_artist_menu_item(title_item, player_get_title(p),
                                              player_get_icon(p), player_get_label(p));
                update_title_artist_menu_item(artist_item, player_get_artist(p),
                                              player_get_icon(p), player_get_label(p));

            } else if (player_status_changed(p)) {
                log_config(MAIN_CTX, "%s not connected\n", player_get_name(p));
            }
        }
        log_config(MAIN_CTX, "%s checked.\n", player_get_name(p));
    }
    return player_get_status(p);
}

int check_radio() {
    if (player_do_check(radio_player)) {
        if (!player_get_status(radio_player)) {
            player_set_status(radio_player, 1);
            apply_radio_theme(ctrl, default_theme);
            update_menu_item_icon_or_label(player_item, player_get_icon(radio_player),
                                           player_get_label(radio_player));
            update_menu_item_icon_or_label(title_item, NULL, "VE 301");
            update_menu_item_icon_or_label(artist_item, NULL, "VE 301");
        }
        song *current_song = get_playing_song();
        if (current_song) {
            log_debug(MAIN_CTX, "Current song: %s\n", current_song->title);
            if (current_song->title && (strlen(current_song->title) > 0) &&
                strncmp(current_song->title, "N/A", 3)) {
                menu_item_update_label(title_item, current_song->title);
            }
            if (current_song->name && (strlen(current_song->name) > 0) &&
                strncmp(current_song->name, "N/A", 3)) {
                menu_item_update_label(artist_item, current_song->name);
            }
        }
    }
    return player_get_status(radio_player);
}

int menu_call_back(menu_ctrl *ctrl) {

    log_debug(MAIN_CTX, "Start: Callback\n");

    time_t timer;
    time(&timer);
    current_tm_info = localtime(&timer);

    long time_diff = timer - info_menu_t;
    long callback_diff = timer - callback_t;

    if (menu_ctrl_get_active(ctrl) != info_menu) {
        current_menu = menu_ctrl_get_active(ctrl);
    }

    log_debug(MAIN_CTX, "Callback diff: %d\n", callback_diff);

    if (check_time_interval(check_internet_interval)) {
        log_info(MAIN_CTX, "Checking internet\n");
        int ia = check_internet();
        if (ia != internet_available) {
            internet_available = ia;
            log_info(MAIN_CTX, "Internet internet became %s\n",
                     internet_available ? "available" : "unavailable");
        }
    }

    int check_state = 0;
#ifdef BLUETOOTH
    if (!check_state) {
        check_state = check_player(ctrl, bluetooth_player, bluetooth_theme);
    }
#endif
#ifdef SPOTIFY
    if (!check_state) {
        check_state = check_player(ctrl, spotify_player, spotify_theme);
    }
#endif

    if (!check_state) {
        check_radio();
    } else {
        player_set_status(radio_player, 0);
    }

    if (callback_diff > CALLBACK_SECONDS) {

        callback_t = timer;

        if (weather_item && temperature_item) {
            char temp_str[255];
            sprintf(temp_str, "%.1f°C", ((double)round(10.0 * wthr.temp)) / 10);
            menu_item_update_label(temperature_item, temp_str);

            if (wthr.weather_icon) {
                menu_item_update_label(weather_item, wthr.weather_icon);
            }
        }

        update_time_item(time_item);
    }

    if (time_diff > info_menu_item_seconds) {
        info_menu_t = timer;
        if (!current_menu || !menu_is_sticky(current_menu)) {
            if (internet_available) {
                char *title_item_label = menu_item_get_label(title_item);
                if (!strcmp(title_item_label, "No internet")) {
                    menu_item_update_label(title_item, "VE 301");
                }

                menu *root = menu_ctrl_get_root(ctrl);

                int current_id = menu_get_current_id(root) + 1;
                if (current_id > menu_get_max_id(root)) {
                    current_id = 0;
                }
                menu_item_warp_to(menu_get_item(root, current_id));

            } else {
                char *title_item_label = menu_item_get_label(title_item);
                if (strcmp(title_item_label, "No internet")) {
                    menu_item_update_label(title_item, "No internet");
                }
                menu_item_warp_to(title_item);
            }
        }
    }

    log_debug(MAIN_CTX, "End:  Callback\n");

    return 1;
}

int weather_lstnr(weather *weather) {
    if (weather) {

        wthr.temp = weather->temp;
        if (wthr.weather_icon) {
            free(wthr.weather_icon);
        }
        wthr.weather_icon = my_copystr((const char *)weather->weather_icon);
        log_config(MAIN_CTX, "Received new weather: %.0f°C\n", weather->temp);
    }

    return 1;
}

#pragma GCC diagnostic ignored "-Waddress"
menu_ctrl *create_menu() {
    radio_config config;

    read_radio_config(&config);

    default_mixer = config.alsa_mixer_name;

    radio_radius_labels = config.radio_radius_labels;

    info_menu_item_seconds = config.info_menu_item_seconds;

    default_theme = get_config_theme("Default");
    bluetooth_theme = get_config_theme("Bluetooth");
    spotify_theme = get_config_theme("Spotify");

    menu_ctrl *ctrl = menu_ctrl_new(config.w,
                                    config.h,
                                    config.x_offset,
                                    config.y_offset,
                                    config.radius_labels,
                                    config.draw_scales,
                                    config.radius_scales_start,
                                    config.radius_scales_end,
                                    config.angle_offset,
                                    config.font,
                                    config.font_size,
                                    config.font_size,
                                    &MENU_ACTION_LISTENER,
                                    &MENU_CALL_BACK);

    if (ctrl) {

        menu_ctrl_enable_mouse(ctrl, 1);

        menu_ctrl_set_light(ctrl,
                            config.light_x,
                            config.light_y,
                            config.light_radius,
                            config.light_alpha);
        if (config.light_img) {
            menu_ctrl_set_light_img(ctrl, config.light_img, config.light_img_x, config.light_img_y);
        }

        menu_ctrl_set_warp_speed(ctrl, config.warp_speed);

        info_menu = menu_new_root(ctrl, 1, config.info_font, config.info_font_size, NULL, 0);
        menu_set_transient(info_menu, 1);

        message_menu = menu_new_root(ctrl, 1, config.info_font, config.info_font_size, NULL, 0);
        menu_set_segments_per_item(message_menu,1);
        message_menu_item =
            menu_item_new(message_menu, "", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL,
                                          0, NULL, NULL, -1);

        menu *root = menu_ctrl_get_root(ctrl);

        player_item = menu_item_new(root, "VE 301", NULL, NULL,
                                    UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);
        title_item = menu_item_new(root, "VE 301", NULL, NULL,
                                   UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);
        artist_item = menu_item_new(root, "VE 301", NULL, NULL,
                                    UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);

        time_t timer;
        time(&timer);
        current_tm_info = localtime(&timer);
        time_item = menu_item_new(root,
                                  NULL,
                                  NULL,
                                  NULL,
                                  OBJ_TYPE_TIME,
                                  config.info_font,
                                  config.time_font_size,
                                  NULL,
                                  config.info_font,
                                  0.6 * config.time_font_size);
        update_time_item(time_item);

        if (strlen(config.weather_api_key) > 0 && strlen(config.weather_location) > 0) {
            init_weather(120, config.weather_api_key, config.weather_location, config.weather_units);
            weather_item = menu_item_new(root,
                                         " ",
                                         NULL,
                                         NULL,
                                         UNKNOWN_OBJECT_TYPE,
                                         config.weather_font,
                                         config.weather_font_size,
                                         NULL,
                                         config.font,
                                         config.font_size);
            temperature_item = menu_item_new(root,
                                             "Weather",
                                             NULL,
                                             NULL,
                                             UNKNOWN_OBJECT_TYPE,
                                             config.info_font,
                                             config.temp_font_size,
                                             NULL,
                                             config.font,
                                             config.font_size);
            start_weather_thread(&weather_lstnr);

        } else {
            weather_item = NULL;
        }

        nav_menu = menu_new_root(ctrl, 1, NULL, 0, NULL, 0);
        menu_ctrl_set_active(ctrl, nav_menu);

        radio_menu = menu_new(
            ctrl,
            get_config_value_int("radio_menu_nm_lines", RADIO_MENU_NO_OF_LINES),
            NULL, 0, NULL, NULL, 0);
        menu_set_segments_per_item(radio_menu, get_config_value_int(
            "radio_menu_segments_per_item", RADIO_MENU_SEGMENTS_PER_ITEM));
        radio_menu_item = menu_add_sub_menu(nav_menu, "Radio", radio_menu, NULL);
        update_radio_menu(ctrl);

        lib_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(nav_menu, "Bibliothek", lib_menu, NULL);
        album_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(lib_menu, "Alben", album_menu, NULL);
        artist_menu = menu_new(ctrl, 3, NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(lib_menu, "Artisten", artist_menu, NULL);

        system_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(nav_menu, "System", system_menu, NULL);

        menu *network_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(system_menu, "Network", network_menu,
                          &item_action_update_network_menu);

        song_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);

        volume_menu = menu_new_root(ctrl, 1, config.info_font, config.info_font_size, NULL, 0);
        menu_set_transient(volume_menu, 1);
        menu_set_draw_only_active(volume_menu, 1);
        if (config.info_bg_image_path) {
            menu_set_bg_image(volume_menu, config.info_bg_image_path);
        }

        mixer_menu = menu_new_root(ctrl, 1, NULL, 0, NULL, 0);
        menu_set_transient(mixer_menu, 1);
        int n_mixers;
        const char **alsa_mixers = alsa_get_mixers(config.mixer_device, &n_mixers);
        for (int n = 0; n < n_mixers; n++) {
            const char *alsa_mixer = alsa_mixers[n];
            menu *mixer_sub_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
            menu_add_sub_menu(mixer_menu, alsa_mixer, mixer_sub_menu, NULL);
            const int volume = alsa_get_volume(alsa_mixers[n]);
            char label[13];
            vol_label(label, volume);
            menu_item_new(
                mixer_sub_menu, label, NULL, alsa_mixer, OBJ_TYPE_MIXER, NULL, -1, NULL, NULL, -1);
            free((void *) alsa_mixer);

        }

        free(alsa_mixers);

        const int volume = alsa_get_volume(default_mixer);
        char label[13];
        vol_label(label, volume);

        volume_menu_item = menu_item_new(
            volume_menu, label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);

        apply_radio_theme(ctrl, default_theme);
    }

    return ctrl;
}

int close_all() {
    log_info(MAIN_CTX, "Closing all\n");
    log_info(MAIN_CTX, "Stopping weather thread\n");
    stop_weather_thread();
    log_info(MAIN_CTX, "Weather thread stopped\n");
#ifdef BLUETOOTH
    log_info(MAIN_CTX, "Stopping bluetooth\n");
    bt_close();
    log_info(MAIN_CTX, "Bluetooth stopped\n");
#endif
#ifdef SPOTIFY
    log_info(MAIN_CTX, "Stopping spotify\n");
    spotify_close();
    log_info(MAIN_CTX, "Spotify stopped\n");
#endif
    if (default_theme) {
        free_theme(default_theme);
        default_theme = NULL;
    }
    if (bluetooth_theme) {
        free_theme(bluetooth_theme);
        bluetooth_theme = NULL;
    }
    if (spotify_theme) {
        free_theme(spotify_theme);
        spotify_theme = NULL;
    }
    log_info(MAIN_CTX, "Stopping audio\n");
    audio_disconnect();
    log_info(MAIN_CTX, "Audio stopped\n");
    menu_ctrl_dispose(ctrl);
    base_close();
    return 0;
}

void sig_handler(int signo) {
    switch (signo) {
    case SIGHUP:
        log_info(MAIN_CTX, "SIGHUP received. Rereading config\n");
        init_config_file("ve301");
        if (default_theme) {
            free_theme(default_theme);
        }
        default_theme = get_config_theme("Default");
        if (bluetooth_theme) {
            free_theme(bluetooth_theme);
        }
        bluetooth_theme = get_config_theme("Bluetooth");
        if (spotify_theme) {
            free_theme(spotify_theme);
        }
        spotify_theme = get_config_theme("Spotify");

        int y_offset = get_config_value_int("y_offset", DEFAULT_Y_OFFSET);
        int x_offset = get_config_value_int("x_offset", DEFAULT_Y_OFFSET);
        double angle_offset =
            get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
        int radius_labels =
            get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
        int radius_scales_start = get_config_value_int("radius_scales_start",
                                                       DEFAULT_SCALES_RADIUS_START);
        int radius_scales_end =
            get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_START);

        menu_ctrl_set_offset(ctrl, x_offset, y_offset);

        menu_ctrl_set_angle_offset(ctrl, angle_offset);

        menu_ctrl_set_radii(ctrl, radius_labels, radius_scales_start,
                            radius_scales_end);

        apply_radio_theme(ctrl, default_theme);

        break;
    default:
        log_error(MAIN_CTX, "Bye with signal %d\n", signo);
        exit(0);
        break;
    }
}

int main(int argc, char **argv) {

    base_init("ve301", stderr, IR_LOG_LEVEL_DEBUG);

    log_info(MAIN_CTX, "Setting signals\n");
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGHUP, sig_handler);

    check_internet_interval = time_check_interval_new(CHECK_INTERNET_SECONDS);

    log_info(MAIN_CTX, "Creating menu... ");
    ctrl = create_menu();
    log_info(MAIN_CTX, "Done\n");

    log_info(MAIN_CTX, "Entering main loop\n");
    if (ctrl) {
        char *radio_icon = get_config_value_path("radio_icon", NULL);
        radio_player = player_new("VE301",
                                  radio_icon,
                                  "VE 301",
                                  get_config_value_int("check_radio_seconds", CHECK_RADIO_SECONDS),
                                  NULL);
        if (player_get_icon(radio_player)) {
            player_set_label(radio_player, NULL);
        }

        free(radio_icon);

#ifdef SPOTIFY
        char *spotify_host = get_config_value("spotify_host", "localhost");
        char *spotify_icon = get_config_value_path("spotify_icon", NULL);
        spotify_player = spotify_init(spotify_host,
                                      "Spotify",
                                      spotify_icon,
                                      get_config_value_int("check_spotify_seconds",
                                                           CHECK_SPOTIFY_SECONDS));
        if (player_get_icon(spotify_player)) {
            player_set_label(spotify_player, NULL);
        }
        free(spotify_host);
        free(spotify_icon);

#endif
#ifdef BLUETOOTH
        char *bluetooth_icon = get_config_value_path("bluetooth_icon", NULL);
        bluetooth_player = bt_init("Bluetooth",
                                   bluetooth_icon,
                                   get_config_value_int("check_bluetooth_seconds",
                                                        CHECK_BLUETOOTH_SECONDS));
        if (player_get_icon(bluetooth_player)) {
            player_set_label(bluetooth_player, NULL);
        }

        free(bluetooth_icon);

#endif
        menu_ctrl_loop(ctrl);
        return close_all();
    } else {
        base_close();
        exit(EXIT_FAILURE);
    }

    return 0;
}
