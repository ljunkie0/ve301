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

#ifndef VE301_RADIO_APP_PRIVATE_H
#define VE301_RADIO_APP_PRIVATE_H

#define _GNU_SOURCE

#include "../radio_app.h"
#ifdef ALSA
#include "../audio/alsa.h"
#endif
#include "../audio/bluetooth.h"
#include "../audio/media_player.h"
#include "../audio/player.h"
#include "../audio/spotify.h"
#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../menu/menu_item.h"
#include "../radio_browser/menu.h"
#include "../sdl_util.h"
#include "../theme.h"
#include "../weather/weather.h"
#include "../wifi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
    char time_menu_item_format[MAX_CONFIG_LINE_LENGTH];
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
    int light_z;
    int light_radius;
    int light_alpha;
    char light_img[MAX_CONFIG_LINE_LENGTH];
    int light_img_x;
    int light_img_y;
    int warp_speed;
    int alsa_enabled;
    char mixer_device[MAX_CONFIG_LINE_LENGTH];
    char alsa_mixer_name[MAX_CONFIG_LINE_LENGTH];
    int info_menu_item_seconds;
    char weather_api_key[MAX_CONFIG_LINE_LENGTH];
    char weather_location[MAX_CONFIG_LINE_LENGTH];
    char weather_units[MAX_CONFIG_LINE_LENGTH];
    int radio_browser_enabled;
    char radio_browser_countrycode[MAX_CONFIG_LINE_LENGTH];
    char radio_browser_server[MAX_CONFIG_LINE_LENGTH];
    char radio_browser_user_agent[MAX_CONFIG_LINE_LENGTH];
    int radio_browser_station_limit;
    int radio_browser_category_limit;
    int radio_browser_language_limit;
} radio_config;

struct radio_app {
    menu_ctrl *ctrl;
    menu *radio_menu;
    menu_item *radio_menu_item;
    menu *info_menu;
    menu *nav_menu;
    menu *lib_menu;
    menu *radio_browser_menu;
    menu *radio_browser_local_menu;
    menu *radio_browser_tag_menu;
    menu *radio_browser_language_menu;
    menu *album_menu;
    menu *artist_menu;
    menu *song_menu;
    menu *system_menu;
    menu *current_menu;
    menu *volume_menu;
    menu *message_menu;
    menu *options_menu;
    menu_item *message_menu_item;
    menu_item *radio_browser_menu_item;
    menu_item *player_icon_item;
    menu_item *player_item;
    menu_item *title_item;
    menu_item *artist_item;
    menu_item *time_item;
    menu_item *weather_item;
    menu_item *temperature_item;
    menu_item *volume_menu_item;
    char *volume_mixer;
    char *time_menu_item_format;
    int alsa_enabled;
    char *default_mixer;
    radio_theme *default_theme;
    player *radio_player;
#ifdef BLUETOOTH
    radio_theme *bluetooth_theme;
    player *bluetooth_player;
#endif

#ifdef SPOTIFY
    radio_theme *spotify_theme;
    player *spotify_player;
#endif
    weather wthr;
    time_t info_menu_t;
    int info_menu_item_seconds;
    time_t callback_t;
    int internet_available;
    time_check_interval *check_internet_interval;
    radio_theme *current_theme;
};

extern struct radio_app *app;

void read_radio_config(radio_config *config);

void apply_radio_theme(radio_theme *theme);

void init_players(const char *radio_player_name, const char *radio_player_label);
void process_player_events(void);
void start_players(void);
void update_player_menu_item(player *p);

void init_info_menu(const radio_config *config);
void radio_app_open_info_menu(void);
void radio_app_touch_activity(int seconds);
void update_info_menu(const time_t timer);
void update_menu_item_label_or_icon(menu_item *item, char *label, char *icon);
void update_time_item(time_t timer);

int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr);
int menu_call_back(menu_ctrl *ctrl);

void init_navigation_menu(const radio_config *config);
void update_radio_menu(void);

int item_action_update_network_menu(menu_event evt, menu *m, menu_item *item);

void init_mixer_menu(const radio_config *config);
void mixer_turn_action(menu *m_ptr, int direction);
void radio_app_show_volume_menu(int volume);
void vol_label(char *buffer, int volume);
#ifdef ALSA
void process_alsa_events(void);
#endif

#endif // VE301_RADIO_APP_PRIVATE_H
