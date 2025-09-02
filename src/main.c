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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>
#include <mpd/client.h>
#include "base.h"
#include "menu/menu.h"
#include "audio/alsa.h"
#include "audio/audio.h"
#include "audio/bluetooth.h"
#ifdef SPOTIFY
#include "audio/spotify.h"
#endif
#include "input_menu.h"
#include "weather.h"
#include "sdl_util.h"
#include "wifi.h"
#include "player.h"

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
#define CHECK_INTERNET_SECONDS 5
#define INFO_MENU_SECONDS 14
#define INFO_MENU_ITEM_SECONDS 5

#define NIGHT_HUE 0.657
#define NIGHT_SAT 0.466
#define NIGHT_VAL 0.161
#define DAY_HUE   0.165
#define DAY_SAT   0.227
#define DAY_VAL   0.971

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

typedef enum {
    ACTIVE, STANDBY
} state;

typedef enum {
    OBJ_TYPE_SONG, OBJ_TYPE_PLAYLIST, OBJ_TYPE_DIRECTORY, OBJ_TYPE_TIME, OBJ_TYPE_MIXER
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
static char *spotify_host;
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

typedef struct radio_theme {
    theme *menu_theme;
    char *info_bg_image_path;
    char *volume_bg_image_path;
    char *info_color;
    char *info_scale_color;
} radio_theme;

static radio_theme *default_theme = NULL;
static radio_theme *bluetooth_theme = NULL;
static radio_theme *spotify_theme = NULL;

static int last_tm_min;
static struct tm* current_tm_info;
static int hsv_style = 0;

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
static menu *root_dir_menu = NULL;
static menu *album_menu = NULL;
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

static playlist *album_songs = NULL;

const char *get_vol_label(int volume) {
    char *vol_txt = malloc(13*sizeof(char));
    sprintf(vol_txt,"Volume: %d%%",volume);
    return vol_txt;
}

int item_action_update_directories_menu(menu_event evt, menu *m, menu_item *item);

station *new_station(const char *label, const char *url) {
    station *s = malloc(sizeof(station));
    s->label = label;
    s->url = url;
    s->id = add_station(url);
    log_info(MAIN_CTX, "Adding station %s with id %d\n", s->label, s->id);
    return s;
}

radio_theme *new_radio_theme(char *info_bg_image_path, char *info_color, char *info_scale_color, char *volume_bg_image_path, theme *menu_theme) {
    radio_theme *theme = malloc(sizeof(radio_theme));
    theme->info_bg_image_path = info_bg_image_path;
    theme->info_color = info_color;
    theme->info_scale_color = info_scale_color;
    theme->volume_bg_image_path = volume_bg_image_path;
    theme->menu_theme = menu_theme;
    return theme;
}

void free_radio_theme(radio_theme *radio_theme) {
    free(radio_theme->info_bg_image_path);
    if (radio_theme->volume_bg_image_path != radio_theme->info_bg_image_path) {
        free(radio_theme->volume_bg_image_path);
    }
    free(radio_theme->info_color);
    free(radio_theme->info_scale_color);
}

void apply_radio_theme(menu_ctrl *ctrl, radio_theme *theme) {

    menu_ctrl_apply_theme (ctrl,theme->menu_theme);

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

    menu_set_colors(info_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(volume_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(message_menu, info_default_clr, info_selected_clr, info_scale_clr);

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

        radio_menu->n_o_lines = RADIO_MENU_NO_OF_LINES;
        radio_menu->segments_per_item = RADIO_MENU_SEGMENTS_PER_ITEM;
        radio_menu->n_o_items_on_scale = RADIO_MENU_ITEMS_ON_SCALE_FACTOR * ctrl->n_o_items_on_scale;
        radio_menu->radius_labels = radio_radius_labels;
        unsigned int r = 0;
        for (r = 0; r < internet_radios->n_songs; r++) {
            song *s = internet_radios->songs[r];
            menu_item_new(radio_menu, s->title, NULL, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
        }

        radio_menu_item->sub_menu = radio_menu;

    } else {

        menu_item_update_label(message_menu_item,"No radio");	    
        message_menu->parent = radio_menu->parent;
        radio_menu_item->sub_menu = message_menu;

        log_error(MAIN_CTX, "create_menu: playlist is NULL\n");

    }

}

void update_album_menu() {
    menu_clear(album_menu);
    unsigned int a;
    char **albums = get_albums(&a);
    if (albums != NULL) {
        log_info(MAIN_CTX, "Album successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_info(MAIN_CTX, "Getting album %d\n", r);
            menu_add_sub_menu(album_menu, albums[r], song_menu, NULL);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: album list is NULL\n");
        menu_item_new(album_menu, "Could not retrieve albums", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_song_menu(menu_item *album_item) {
    log_info(MAIN_CTX, "Update song menu: %s\n", album_item->unicode_label);
    int id = 0;
    for (id = 0; id <= song_menu->max_id; id++) {
        dispose_song((song *)song_menu->item[id]->object);
    }
    menu_clear(song_menu);
    if (album_songs) {
        dispose_playlist(album_songs);
    }
    char *album_label = (char *) album_item->unicode_label;
    album_songs = get_album_songs(album_label);
    if (album_songs != NULL) {
        unsigned int r = 0;
        for (r = 0; r < album_songs->n_songs; r++) {
            song *s = album_songs->songs[r];
            log_info(MAIN_CTX, "Song: %s\n", s->title);
            menu_item_new((menu *) album_item->sub_menu, s->title, NULL, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
        }
    } else {
        log_error(MAIN_CTX, "update_song_menu: playlist is NULL\n");
        menu_item_new((menu *)album_item->sub_menu, "FEHLER", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_directory_menu(menu_item *directory_item) {
    menu *m = (menu *) directory_item->menu;
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    if (directory_item->sub_menu) {
        menu_clear((menu *)directory_item->sub_menu);
    } else {
        directory_item->sub_menu = menu_new((menu_ctrl *)m->ctrl, m->n_o_lines, NULL,0,NULL,NULL,0);
    }
    menu *sub_menu = (menu *) directory_item->sub_menu;
    struct mpd_connection *mpd_conn = get_mpd_connection();
    if (mpd_conn) {
        if (directory_item->object_type != OBJ_TYPE_DIRECTORY) {
            log_error(MAIN_CTX, "Item is not of type directory\n");
            return;
        }
        const char *dir = (const char *) directory_item->object;
        if (!mpd_send_list_meta(mpd_conn,dir)) {
            log_error(MAIN_CTX, "Could not list directory %s\n", dir);
            return;
        }
        struct mpd_entity *entity = mpd_recv_entity(mpd_conn);
        while (entity) {
            log_info(MAIN_CTX, "Type: %d\n", mpd_entity_get_type(entity));
            enum mpd_entity_type type = mpd_entity_get_type(entity);
            if (type == MPD_ENTITY_TYPE_DIRECTORY) {
                const struct mpd_directory *sub_dir = mpd_entity_get_directory(entity);
                const char *path = mpd_directory_get_path(sub_dir);
                char *name = get_name_from_path(path);
                log_info(MAIN_CTX, "Directory %s\n", name);
                menu *sub_sub_menu = menu_new(ctrl,m->n_o_lines,NULL,0,NULL,NULL,0);
                menu_item *sub_sub_menu_item = menu_add_sub_menu(sub_menu,name,sub_sub_menu, item_action_update_directories_menu);
                sub_sub_menu_item->object = (const void *) path;
                sub_sub_menu_item->object_type = OBJ_TYPE_DIRECTORY;
            } else if (type == MPD_ENTITY_TYPE_SONG) {
                const struct mpd_song *s = mpd_entity_get_song(entity);
                const char *uri = mpd_song_get_uri(s);
                char *name = get_name_from_path(uri);
                log_info(MAIN_CTX, "Song: %s\n", name);
                char *title = (char *) mpd_song_get_tag(s,MPD_TAG_TITLE,0);
                song *my_song = new_song(mpd_song_get_id(s),uri,name,title);
                menu_item_new(sub_menu,my_song->title,NULL,my_song,OBJ_TYPE_SONG,NULL,-1, NULL, NULL, -1);
                free((char *) uri);
                free(name);
                free(title);
            }
            entity = mpd_recv_entity(mpd_conn);
        }
    } else {

    }
}

void update_time_item(menu_item *time_menu_item) {
    last_tm_min = current_tm_info->tm_min;
    char *buffer = malloc(25 * sizeof(char));
    strftime(buffer, 25, time_menu_item_format, current_tm_info);
    menu_item_update_label(time_menu_item, buffer);
    log_debug(MAIN_CTX, "free(buffer))\n");
    log_debug(MAIN_CTX, "buffer = %p\n", buffer);
    free(buffer);
}

int item_action_update_directories_menu(menu_event evt, menu *m, menu_item *item) {
    update_directory_menu(item);
    return 0;
}

int menu_action_listener_dbg(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    return 0;
}

int item_action_update_interface_menu(menu_event evt, menu *m, menu_item *item) {
    if (evt == DISPOSE) {
        if (item->object) {
            network_interface *interface = (network_interface *) item->object;
            free_network_interface(interface);
        }
    } else if (item->sub_menu) {
        menu_clear(item->sub_menu);

        network_interface *interface = (network_interface *) item->object;
        char *ip_address_label = my_catstr("IP\n", interface->ipaddress);
        menu_item_new(item->sub_menu, ip_address_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        free(ip_address_label);

        log_debug(MAIN_CTX, "Getting result from wifi scan\n");
        struct station_info *wifi_scan_result = scan_wifi_station(interface->ifname);
        log_debug(MAIN_CTX, "Result: %p\n", wifi_scan_result);

        if (wifi_scan_result) {
            char *ssid_label = my_catstr("Station\n", wifi_scan_result->ssid);
            log_debug(MAIN_CTX, "%s\n", ssid_label);
            menu_item_new(item->sub_menu, ssid_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
            free(ssid_label);

            char signal_chr[100];
            sprintf(signal_chr, "%d dBm", wifi_scan_result->signal_dbm);
            char *strength_label = my_catstr("Strength\n", signal_chr);
            menu_item_new(item->sub_menu, strength_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
            free(strength_label);

            free(wifi_scan_result);
            log_debug(MAIN_CTX, "Done\n");
        }

    }

    return 0;

}

int item_action_update_network_menu(menu_event evt, menu *m, menu_item *item) {

    if (evt == DISPOSE) {
        return 0;
    }

    if (item->sub_menu) {

        int id = item->sub_menu->max_id;
        while (id >= 0) {
            if (item->sub_menu->item[id]->object) {
                free((void *) item->sub_menu->item[id]->object);
                item->sub_menu->item[id]->object = 0;
            }
            id--;
        }
        menu_clear(item->sub_menu);
    }

    network_interfaces *interfaces = get_network_interfaces();
    if (interfaces) {
        for (int i = 0; i < interfaces->n; i++) {
            network_interface *interface = interfaces->interfaces[i];
            menu *interface_menu = menu_new(m->ctrl, 1, NULL, 0, NULL, NULL, 0);
            interface_menu->n_o_items_on_scale = 3;
            menu_item *interface_item = menu_add_sub_menu(item->sub_menu, interface->ifname, interface_menu, &item_action_update_interface_menu);
            interface_item->object = interface;
        }
    }

    return 0;

}

void input_menu_ok(menu *input_menu, char *input) {
    log_warning(MAIN_CTX, "Input: %s\n", input);
}

int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    if (log_level_enabled(MAIN_CTX, IR_LOG_LEVEL_CONFIG)) {
        char *label = item_ptr ? item_ptr->label : "";
        log_config(MAIN_CTX, "action(%d, %p, %s)\n",evt,m_ptr,label);
    }
    if (evt == ACTIVATE) {
        if (item_ptr) {
            menu_item *item = (menu_item *) item_ptr;
            log_config(MAIN_CTX, "Action: %s\n", item->label);
            if (item->sub_menu) {
                log_config(MAIN_CTX, "Sub menu: %s\n", item->label);
                if (item == radio_menu_item) {
                    update_radio_menu(ctrl);
                } else if (item->sub_menu == album_menu) {
                    update_album_menu();
                } else if (item->sub_menu == song_menu) {
                    update_song_menu(item);
                } else if (item->object_type == OBJ_TYPE_DIRECTORY || item->sub_menu == root_dir_menu) {
                    update_directory_menu(item);
                }
            } else if (item->object && item->object_type == OBJ_TYPE_SONG) {
                m_ptr->active_id = item->id;
                song *s = (song *) item->object;
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
            } else if (((menu *)item->menu)->transient) {
                if (ctrl->active) {
                    menu_open(ctrl->active);
                } else {
                    menu_open(nav_menu);
                }
            }
        } else if (m_ptr && ((menu *)m_ptr)->transient) {
            if (ctrl->active) {
                menu_open(ctrl->active);
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
        } else if (m_ptr && m_ptr->current_id >= 0 && m_ptr->item[m_ptr->current_id]->object_type == OBJ_TYPE_MIXER) {
            menu_item *mixer_item = m_ptr->item[m_ptr->current_id];
            char *mixer_name = (char *) mixer_item->object;
            int vol = alsa_get_volume(mixer_name);
            log_config(MAIN_CTX, "Current volume of mixer %s: %d\n", mixer_name, vol);
            alsa_set_volume(default_mixer, vol-2);
            log_config(MAIN_CTX, "Decreased volume: %d\n", vol);
            const char *vol_label = get_vol_label(vol);
            menu_item_update_label(mixer_item,vol_label);
        } else {
            menu_item_show(volume_menu_item);
            int vol = alsa_get_volume(default_mixer);
            log_config(MAIN_CTX, "Current volume: %d\n", vol);
            alsa_set_volume(default_mixer, vol-2);
            vol = alsa_get_volume(default_mixer);
            log_config(MAIN_CTX, "Decreased volume: %d\n", vol);
            const char *vol_label = get_vol_label(vol);
            menu_item_update_label(volume_menu_item,vol_label);
            menu_item_show(volume_menu_item);
        }
    } else if (evt == TURN_RIGHT_1) {
        if (m_ptr == mixer_menu) {
            menu_turn_right((menu *)m_ptr);
        } else {
            menu_item_show(volume_menu_item);
            int vol = alsa_get_volume(default_mixer);
            alsa_set_volume(default_mixer,vol+2);
            vol = alsa_get_volume(default_mixer);
            const char *vol_label = get_vol_label(vol);
            menu_item_update_label(volume_menu_item,vol_label);
            menu_item_show(volume_menu_item);
        }
    } else if (evt == DISPOSE) {
        menu_item *item = (menu_item *) item_ptr;
        const void *obj = item->object;
        if (obj && item->menu != radio_menu) {
            if (item->object_type == OBJ_TYPE_DIRECTORY) {
                log_debug(MAIN_CTX, "free(%p)\n",obj);
                free((void *)obj);
            } else if (item->object_type == OBJ_TYPE_SONG) {
                dispose_song((song *)obj);
            }
        }
    }

    reset_info_menu_timer();

    return 0;
}

radio_theme *get_config_theme(const char *theme_name) {
    theme *menu_theme = malloc (sizeof(theme));
    menu_theme->background_color = get_config_value_group("background_color", get_config_value("background_color", "#ffffff"), theme_name);
    menu_theme->scale_color = get_config_value_group("scale_color", get_config_value("scale_color", "#ff0000"), theme_name);
    menu_theme->indicator_color = get_config_value_group("indicator_color", get_config_value("indicator_color", "#ff0000"), theme_name);
    menu_theme->default_color = get_config_value_group("default_color", get_config_value("default_color", "#00ff00"), theme_name);
    menu_theme->selected_color = get_config_value_group("selected_color", get_config_value("selected_color", "#0000ff"), theme_name);
    menu_theme->activated_color = get_config_value_group("activated_color", get_config_value("activated_color", "#00ffff"), theme_name);
    menu_theme->bg_image_path = get_config_value_path_group("bg_image_path", get_config_value("bg_image_path", 0), theme_name);
    menu_theme->font_bumpmap = get_config_value_int_group("font_bumpmap", get_config_value_int("font_bumpmap", 0), theme_name);
    menu_theme->shadow_offset = get_config_value_int_group("shadow_offset",get_config_value_int("shadow_offset",0), theme_name);
    menu_theme->shadow_alpha = get_config_value_int_group("shadow_alpha", get_config_value_int("shadow_alpha", 0), theme_name);
    menu_theme->bg_cp_colors = 0;
    menu_theme->fg_color_palette = NULL;
    menu_theme->fg_cp_colors = 0;
    menu_theme->bg_color_palette = NULL;

    char *info_menu_bg_path = get_config_value_path_group("info_bg_image_path", get_config_value("info_bg_image_path", 0), theme_name);
    char *volume_menu_bg_path = get_config_value_group("volume_bg_image_path", info_menu_bg_path, theme_name);

    char *info_color = get_config_value_group("info_color", NULL, "Default");
    char *info_scale_color = get_config_value_group("info_scale_color", NULL, "Default");

    log_config(MAIN_CTX, "Radio theme %s:\n", theme_name);
    log_config(MAIN_CTX, "Background color:              %s\n", menu_theme->background_color);
    log_config(MAIN_CTX, "Scale color:                   %s\n", menu_theme->scale_color);
    log_config(MAIN_CTX, "Indicator color:               %s\n", menu_theme->indicator_color);
    log_config(MAIN_CTX, "Default color:                 %s\n", menu_theme->default_color);
    log_config(MAIN_CTX, "Selected color:                %s\n", menu_theme->selected_color);
    log_config(MAIN_CTX, "Activated color:               %s\n", menu_theme->activated_color);
    log_config(MAIN_CTX, "Background image:              %s\n", menu_theme->bg_image_path);
    log_config(MAIN_CTX, "Font bumpmap:                  %d\n", menu_theme->font_bumpmap);
    log_config(MAIN_CTX, "Shadow offset:                 %d\n", menu_theme->shadow_offset);
    log_config(MAIN_CTX, "Shadow alpha:                  %d\n", menu_theme->shadow_alpha);
    log_config(MAIN_CTX, "Info menu background image:    %s\n", info_menu_bg_path);
    log_config(MAIN_CTX, "Info color:                    %s\n", info_color);
    log_config(MAIN_CTX, "Info scale color:              %s\n", info_scale_color);
    log_config(MAIN_CTX, "Volume menu background image:  %s\n", volume_menu_bg_path);

    return new_radio_theme(info_menu_bg_path, info_color, info_scale_color, volume_menu_bg_path, menu_theme);
}

void free_theme(radio_theme *rth) {

    theme *th = rth->menu_theme;

    log_debug(MAIN_CTX, "free_theme(%p)\n",th);
    if (th->background_color)
        free(th->background_color);
    if (th->scale_color)
        free(th->scale_color);
    if (th->indicator_color)
        free(th->indicator_color);
    if (th->default_color)
        free(th->default_color);
    if (th->selected_color)
        free(th->selected_color);
    if (th->activated_color)
        free(th->activated_color);
    if (th->bg_image_path)
        free(th->bg_image_path);
    if (th->bg_color_palette)
        free(th->bg_color_palette);
    if (th->fg_color_palette)
        free(th->fg_color_palette);
    free(th);

    free_radio_theme(rth);

}

static time_t info_menu_t_dbg = -1;

static long perf_timer_ms = 0;
// #define SECS_IN_DAY (24 * 60 * 60)

int menu_call_back_dbg(menu_ctrl *m_ptr) {

    menu_ctrl *ctrl = (menu_ctrl *) m_ptr;

    if (ctrl->current != radio_menu) {
        menu_open(radio_menu);
    }

    if (ctrl && ctrl->current) {
        int current_id = ctrl->current->current_id+1;
        if (current_id > ctrl->current->max_id) {
            current_id = 0;
        }

        struct timespec perf_timer_s, perf_timer_e;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &perf_timer_s);
        long msecs_s = perf_timer_s.tv_sec * 1000 + perf_timer_s.tv_nsec / 1000000;
        menu_item_warp_to(ctrl->current->item[current_id]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &perf_timer_e);
        long msecs_e = perf_timer_e.tv_sec * 1000 + perf_timer_e.tv_nsec / 1000000;

        long msecs_p = msecs_e - msecs_s;
        perf_timer_ms += msecs_p;

        time_t timer;
        time(&timer);
        current_tm_info = localtime(&timer);

        if (info_menu_t_dbg < 0) {
            info_menu_t_dbg = timer;
        }

        long time_diff = timer - info_menu_t_dbg;

        if (time_diff > 30) {
            printf("Time: %ld\n", perf_timer_ms);
            menu_ctrl_quit(ctrl);
            base_close();
            exit(0);
        }
    }

    return 1;
}

void update_menu_item_icon_or_label(menu_item *item, char *icon, char *label) {
    if (icon) {
        menu_item_update_icon(item,icon);
        menu_item_update_label(item,NULL);
    } else {
        menu_item_update_label(item,label);
        menu_item_update_icon(item,NULL);
    }
}

void update_title_artist_menu_item(menu_item *item, char *label, char *icon, char *label_default) {
    if (label) {
        update_menu_item_icon_or_label(item, NULL, label);
    } else if (icon) {
        update_menu_item_icon_or_label(item, icon, NULL);
    } else {
        update_menu_item_icon_or_label(item, NULL, label_default);
    }
}

#ifdef BLUETOOTH
int check_bluetooth(menu_ctrl *ctrl) {
    if (player_do_check(bluetooth_player)) {
        log_config(MAIN_CTX, "Checking bluetooth...\n");
        if (bt_connection_signal(/* proces_prop_changes => */ 1)) {
            if (bt_is_connected()) {
                log_config(MAIN_CTX, "Bluetooth is connected.\n");
                if (!player_get_status(bluetooth_player)) {
                    player_set_status(bluetooth_player,1);
                    stop();
                    if (bluetooth_theme) {
                        apply_radio_theme(ctrl, bluetooth_theme);
                    }
                    update_menu_item_icon_or_label(player_item, player_get_icon(bluetooth_player), player_get_label(bluetooth_player));
                }

                update_title_artist_menu_item(title_item, bt_get_title(), player_get_icon(bluetooth_player), player_get_label(bluetooth_player));
                update_title_artist_menu_item(artist_item, bt_get_artist(), player_get_icon(bluetooth_player), player_get_label(bluetooth_player));

            } else {
                log_config(MAIN_CTX, "Bluetooth not connected\n");
                player_set_status(bluetooth_player,0);
            }
        }
        log_config(MAIN_CTX, "Bluetooth checked.\n");
    }
    return player_get_status(bluetooth_player);
}
#endif

#ifdef SPOTIFY
int check_spotify(menu_ctrl *ctrl) {

    if (player_do_check(spotify_player)) {
        log_config(MAIN_CTX, "Checking spotify...\n");

        if (spotify_is_connected()) {
            log_config(MAIN_CTX, "Spotify is connected\n");
            if (!player_get_status(spotify_player)) {
                log_config(MAIN_CTX, "Spotify activated\n");
                player_set_status(spotify_player,1);
                stop();
                if (spotify_theme) {
                    apply_radio_theme(ctrl, spotify_theme);
                }
                update_menu_item_icon_or_label(player_item, player_get_icon(spotify_player), player_get_label(spotify_player));
            }

            update_title_artist_menu_item(title_item, spotify_get_title(), player_get_icon(spotify_player), player_get_label(spotify_player));
            update_title_artist_menu_item(artist_item, spotify_get_artist(), player_get_icon(spotify_player), player_get_label(spotify_player));

        } else {
            log_config(MAIN_CTX, "Spotify not connected\n");
            player_set_status(spotify_player,0);
        }
        log_config(MAIN_CTX, "spotify checked.\n");
    }

    return player_get_status(spotify_player);

}
#endif

int check_radio() {
    if (player_do_check(radio_player)) {
        if (!player_get_status(radio_player)) {
            player_set_status(radio_player,1);
            apply_radio_theme(ctrl, default_theme);
            update_menu_item_icon_or_label(player_item, player_get_icon(radio_player), player_get_label(radio_player));
            update_menu_item_icon_or_label(title_item, NULL, "VE 301");
            update_menu_item_icon_or_label(artist_item, NULL, "VE 301");
        }
        song *current_song = get_playing_song();
        if (current_song) {
            log_debug(MAIN_CTX, "Current song: %s\n", current_song->title);
            if (current_song->title && (strlen(current_song->title) > 0) && strncmp(current_song->title, "N/A", 3)) {
                menu_item_update_label(title_item, current_song->title);
            }
            if (current_song->name && (strlen(current_song->name) > 0) && strncmp(current_song->name, "N/A", 3)) {
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
 

    if (ctrl->active != info_menu) {
        current_menu = ctrl->active;
    }

    log_debug(MAIN_CTX, "Callback diff: %d\n", callback_diff);

    if (check_time_interval(check_internet_interval)) {
        internet_available = check_internet();
    }

    int check_state = 0;
#ifdef BLUETOOTH
    if (!check_state) {
        check_state = check_bluetooth(ctrl);
    }
#endif
#ifdef SPOTIFY
    if (!check_state) {
        check_state = check_spotify(ctrl);
    }
#endif

    if (!check_state) {
        check_radio();
    } else {
        player_set_status(radio_player,0); 
    }

    if (callback_diff > CALLBACK_SECONDS) {

        callback_t = timer;

        if (weather_item && temperature_item) {
            char temp_str[255];
            sprintf(temp_str,"%.1f°C",((double)round(10.0*wthr.temp))/10);
            menu_item_update_label(temperature_item,temp_str);

            if (wthr.weather_icon) {
                menu_item_update_label(weather_item,wthr.weather_icon);
            }
        }

        update_time_item(time_item);

    }

    if (time_diff > info_menu_item_seconds) {
        info_menu_t = timer;
        if (!current_menu || !current_menu->sticky) {
            if (internet_available) {
                char *title_item_label = menu_item_get_label(title_item);
                if (!strcpy(title_item_label,"No internet")) {
                    menu_item_update_label(title_item,"VE 301");
                }
                int current_id = ctrl->root[0]->current_id+1;
                if (current_id > ctrl->root[0]->max_id) {
                    current_id = 0;
                }
                menu_item_warp_to(ctrl->root[0]->item[current_id]);
            } else {
                menu_item_update_label(title_item,"No internet");
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
        wthr.weather_icon = my_copystr((const char *) weather->weather_icon);
        log_config(MAIN_CTX, "Received new weather: %.0f°C\n",weather->temp);

    }

    return 1;

}

#pragma GCC diagnostic ignored "-Waddress"
menu_ctrl *create_menu() {

    char *font = get_config_value_path("font", DEFAULT_FONT);
    char *info_font = get_config_value_path("info_font", font);
    char *info_bg_image_path = get_config_value_path("info_bg_image_path", NULL);
    int font_size = get_config_value_int("font_size",DEFAULT_FONT_SIZE);
    int info_font_size = get_config_value_int("info_font_size",DEFAULT_INFO_FONT_SIZE);
    char *weather_font = get_config_value_path("weather_font", DEFAULT_FONT);
    int weather_font_size = get_config_value_int("weather_font_size", info_font_size);
    int temp_font_size = get_config_value_int("temperature_font_size", info_font_size);
    int time_font_size = get_config_value_int("time_font_size",info_font_size);
    int w = get_config_value_int("window_width",DEFAULT_WINDOW_WIDTH);
    int y_offset = get_config_value_int("y_offset",DEFAULT_Y_OFFSET);
    int x_offset = get_config_value_int("x_offset",DEFAULT_Y_OFFSET);
    double angle_offset = get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
    int radius_labels = get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
    int radius_scales_start = get_config_value_int("radius_scales_start", DEFAULT_SCALES_RADIUS_START);
    int radius_scales_end = get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_END);
    int draw_scales = get_config_value_int("draw_scales",1);
    int light_x = get_config_value_int("light_x", (int) w/2);
    int light_y = get_config_value_int("light_y", 100);
    int light_radius = get_config_value_int("light_radius", 300);
    int light_alpha = get_config_value_int("light_alpha", 0);
    char *light_img = get_config_value_path("light_image_path",NULL);
    int light_img_x = get_config_value_int("light_image_x", 0);
    int light_img_y = get_config_value_int("light_image_y", 0);
    int warp_speed = get_config_value_int("warp_speed", 10);
    char *mixer_device = get_config_value_path("alsa_mixer_device", "default");
    default_mixer = get_config_value("alsa_mixer_name", "Master");
    radio_radius_labels = get_config_value_int("radio_radius_labels", radius_labels);

    radio_player = player_new(
            get_config_value_path("radio_icon",NULL),
            "VE 301",
            get_config_value_int("check_radio_seconds", CHECK_RADIO_SECONDS)
    );
    if (player_get_icon(radio_player)) {
        player_set_label(radio_player,NULL);
    }
#ifdef SPOTIFY
    spotify_player = player_new(
            get_config_value_path("spotify_icon",NULL),
            "Spotify",
            get_config_value_int("check_spotify_seconds", CHECK_SPOTIFY_SECONDS)
    );
    if (player_get_icon(spotify_player)) {
            player_set_label(spotify_player,NULL);
    }
    spotify_host = get_config_value("spotify_host","localhost");
#endif
#ifdef BLUETOOTH
    bluetooth_player = player_new(
            get_config_value_path("bluetooth_icon",NULL),
            "Bluetooth",
            get_config_value_int("check_bluetooth_seconds", CHECK_BLUETOOTH_SECONDS)
    );
    if (player_get_icon(bluetooth_player)) {
        player_set_label(bluetooth_player,NULL);
    }
#endif

    info_menu_item_seconds = get_config_value_int("info_menu_item_seconds",INFO_MENU_ITEM_SECONDS);
    hsv_style = get_config_value_int("hsv_style",0);

    default_theme = get_config_theme ("Default");
    bluetooth_theme = get_config_theme("Bluetooth");
    spotify_theme = get_config_theme("Spotify");

    menu_ctrl *ctrl = menu_ctrl_new (
            w,
            x_offset,
            y_offset,
            radius_labels,
            draw_scales,
            radius_scales_start,
            radius_scales_end,
            angle_offset,
            font,
            font_size,
            font_size,
            &MENU_ACTION_LISTENER,
            &MENU_CALL_BACK
            );

    if (ctrl) {

        menu_ctrl_enable_mouse(ctrl, 1);

        menu_ctrl_set_light(ctrl,light_x,light_y,light_radius,light_alpha);
        if (light_img) {
            menu_ctrl_set_light_img(ctrl,light_img,light_img_x,light_img_y);
            free(light_img);
        }

        menu_ctrl_set_warp_speed(ctrl, warp_speed);

        info_menu = menu_new_root(ctrl, 1, info_font, info_font_size, NULL, 0);
        info_menu->transient = 1;

        message_menu = menu_new_root(ctrl,1, info_font, info_font_size, NULL, 0);
        message_menu->segments_per_item = 1;
        message_menu_item = menu_item_new(message_menu, "", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);

        player_item = menu_item_new(ctrl->root[0], "VE 301", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);
        title_item = menu_item_new(ctrl->root[0], "VE 301", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);
        artist_item = menu_item_new(ctrl->root[0], "VE 301", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);

        time_t timer;
        time(&timer);
        struct tm *tm_info = localtime(&timer);
        last_tm_min = tm_info->tm_min;
        char *buffer = malloc(25 * sizeof(char));
        strftime(buffer, 25, time_menu_item_format, tm_info);
        time_item = menu_item_new(ctrl->root[0], buffer, NULL, NULL, OBJ_TYPE_TIME, info_font, time_font_size, NULL, info_font, 0.6*time_font_size);
        free(buffer);

        const char *weather_api_key = get_config_value("weather_api_key", "");
        const char *weather_location = get_config_value("weather_location", "");
        const char *weather_units = get_config_value("weather_units", "metric");
        if (strlen(weather_api_key) > 0 && strlen(weather_location) > 0) {
            init_weather(120,weather_api_key,weather_location,weather_units);
            weather_item = menu_item_new(ctrl->root[0], " ", NULL, NULL, UNKNOWN_OBJECT_TYPE, weather_font, weather_font_size, NULL, font, font_size);
            temperature_item = menu_item_new(ctrl->root[0], "Weather", NULL, NULL, UNKNOWN_OBJECT_TYPE, info_font, temp_font_size, NULL, font, font_size);
            start_weather_thread(&weather_lstnr);

        } else {
            weather_item = NULL;
        }

        if (weather_font != DEFAULT_FONT) {
            free(weather_font);
        }

        nav_menu = menu_new_root(ctrl, 1, NULL, 0, NULL, 0);
        ctrl->active = nav_menu;

        radio_menu = menu_new(ctrl, RADIO_MENU_NO_OF_LINES, NULL, 0, NULL, NULL, 0);
        radio_menu_item = menu_add_sub_menu(nav_menu, "Radio", radio_menu, NULL);
        update_radio_menu(ctrl);

        lib_menu = menu_new(ctrl,1,NULL, 0, NULL, NULL, 0);
        menu_add_sub_menu(nav_menu, "Bibliothek", lib_menu, NULL);
        album_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);
        menu_add_sub_menu(lib_menu, "Alben", album_menu, NULL);

        system_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);
        menu_add_sub_menu(nav_menu, "System", system_menu, NULL);

        menu *input_menu = input_menu_new(ctrl, info_font, info_font_size, &input_menu_ok);
        input_menu->sticky = 1;
        menu_add_sub_menu(nav_menu, "Input", input_menu, NULL);

        menu *network_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);
        menu_add_sub_menu(system_menu, "Network", network_menu, &item_action_update_network_menu);

        root_dir_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);
        menu_item *dir_menu_item = menu_add_sub_menu(nav_menu, "Verzeichnisse", root_dir_menu, item_action_update_directories_menu);
        dir_menu_item->object = my_copystr("/");

        song_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);

        volume_menu = menu_new_root(ctrl,1, info_font, info_font_size, NULL, 0);
        volume_menu->transient = 1;
        volume_menu->draw_only_active = 1;
        if (info_bg_image_path) {
            menu_set_bg_image(volume_menu, info_bg_image_path);
            free(info_bg_image_path);
        }

        mixer_menu = menu_new_root(ctrl,1,NULL,0,NULL,0);
        mixer_menu->transient = 1;
        int n_mixers;
        const char **alsa_mixers = alsa_get_mixers(mixer_device, &n_mixers);
        for (int n = 0; n < n_mixers; n++) {
            menu *mixer_sub_menu = menu_new(ctrl,1,NULL,0,NULL,NULL,0);
            menu_add_sub_menu(mixer_menu, alsa_mixers[n], mixer_sub_menu, NULL);	
            const int volume = alsa_get_volume(alsa_mixers[n]);
            const char *vol_label = get_vol_label(volume);
            menu_item_new(mixer_sub_menu, vol_label, NULL, alsa_mixers[n], OBJ_TYPE_MIXER, NULL, -1, NULL, NULL, -1);
        }

        const int volume = alsa_get_volume(default_mixer);
        const char *vol_label = get_vol_label(volume);

        volume_menu_item = menu_item_new(volume_menu, vol_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        free((char *) vol_label);

        apply_radio_theme (ctrl,default_theme);
    }

    if (font != DEFAULT_FONT) {
        free(font);
    }

    return ctrl;
}

int close_all() {
    stop_weather_thread();
#ifdef BLUETOOTH
    bt_close();
#endif
#ifdef SPOTIFY
    spotify_close();
#endif
    if (bluetooth_theme) {
        free_theme(bluetooth_theme);
    }
    if (spotify_theme) {
        free_theme(spotify_theme);
    }
    audio_disconnect();
    base_close();
    menu_ctrl_dispose(ctrl);
    return 0;
}

void sig_handler(int signo) {
    switch(signo) {
        case SIGHUP:
            log_info(MAIN_CTX, "SIGHUP received. Rereading config\n");
            init_config_file("ve301");
            if (default_theme) {
                free_theme(default_theme);
            }
            default_theme = get_config_theme ("Default");
            if (bluetooth_theme) {
                free_theme(bluetooth_theme);
            }
            bluetooth_theme = get_config_theme ("Bluetooth");
            if (spotify_theme) {
                free_theme(spotify_theme);
            }
            spotify_theme = get_config_theme ("Spotify");

            int y_offset = get_config_value_int("y_offset",DEFAULT_Y_OFFSET);
            int x_offset = get_config_value_int("x_offset",DEFAULT_Y_OFFSET);
            double angle_offset = get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
            int radius_labels = get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
            int radius_scales_start = get_config_value_int("radius_scales_start", DEFAULT_SCALES_RADIUS_START);
            int radius_scales_end = get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_START);

            ctrl->y_offset = y_offset;
            ctrl->x_offset = x_offset;
            ctrl->angle_offset = angle_offset;

            menu_ctrl_set_radii(ctrl,radius_labels,radius_scales_start,radius_scales_end);

            apply_radio_theme(ctrl,default_theme);

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
#ifdef BLUETOOTH
        bt_init();
#endif
#ifdef SPOTIFY
        spotify_init(spotify_host);
#endif
        menu_ctrl_loop(ctrl);
        return 0;
    } else {
        base_close();
        exit(EXIT_FAILURE);
    }

    return 0;

}

