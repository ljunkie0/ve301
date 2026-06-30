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

#define _GNU_SOURCE

#include "radio_app.h"
#ifdef ALSA
#include "audio/alsa.h"
#endif
#include "audio/bluetooth.h"
#include "audio/media_player.h"
#include "audio/player.h"
#include "audio/spotify.h"
#include "base/config.h"
#include "base/log_contexts.h"
#include "base/logging.h"
#include "base/util.h"
#include "menu/menu_item.h"
#include "radio_browser_menu.h"
#include "sdl_util.h"
#include "theme.h"
#include "weather.h"
#include "wifi.h"
#include <unistd.h>

#define PLAYER_EVENT_STATE 1
#define PLAYER_EVENT_METADATA 2
#define PLAYER_EVENT_COVER 4
#define PLAYER_EVENT_VOLUME 8
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
    int info_menu_item_seconds;
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
    time_t callback_t;
    int internet_available;
    time_check_interval *check_internet_interval;
    radio_theme *current_theme;
};

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

static struct radio_app *app;

static int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr);
static void radio_app_show_volume_menu(int volume);

static void radio_app_touch_activity(void) {
    time_t timer;
    time(&timer);
    app->info_menu_t = timer;
    log_debug(MAIN_CTX, "radio_app_touch_activity: app->info_menu_t = %d\n", app->info_menu_t);
}

static void read_radio_config(radio_config *config) {
    config_value_path(config->font, "font", DEFAULT_FONT);
    config->font_size = get_config_value_int("font_size", DEFAULT_FONT_SIZE);
    config_value_path(config->info_font, "info_font", config->font);
    config->info_font_size = get_config_value_int("info_font_size", DEFAULT_INFO_FONT_SIZE);
    config_value_path(config->info_bg_image_path, "info_bg_image_path", NULL);
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
    config->light_z = get_config_value_int("light_z", 0);
    config->light_radius = get_config_value_int("light_radius", 300);
    config->light_alpha = get_config_value_int("light_alpha", 0);
    config_value_path(config->light_img, "light_image_path", NULL);
    config->light_img_x = get_config_value_int("light_image_x", 0);
    config->light_img_y = get_config_value_int("light_image_y", 0);
    config->warp_speed = get_config_value_int("warp_speed", 10);
    config->radio_radius_labels = get_config_value_int("radio_radius_labels", config->radius_labels);
    config->info_menu_item_seconds = get_config_value_int("info_menu_item_seconds",
                                                          INFO_MENU_ITEM_SECONDS);
#ifdef ALSA
    config->alsa_enabled = get_config_value_int("alsa_enabled", 0);
    config_value(config->mixer_device, "alsa_mixer_device", "default");
    config_value(config->alsa_mixer_name, "alsa_mixer_name", "Master");
#else
    config->alsa_enabled = 0;
    config->mixer_device = NULL;
#endif
    config_value(config->weather_api_key, "weather_api_key", "");
    config_value(config->weather_location, "weather_location", "");
    config_value(config->weather_units, "weather_units", "metric");
    config_value(config->time_menu_item_format, "time_menu_item_format", "%H:%M\n%d. %B");
    config->radio_browser_enabled = get_config_value_int("radio_browser_enabled", 1);
    config_value(config->radio_browser_countrycode, "radio_browser_countrycode", "DE");
    config_value(config->radio_browser_server,
                 "radio_browser_server",
                 "https://de1.api.radio-browser.info");
    config_value(config->radio_browser_user_agent, "radio_browser_user_agent", "VE301");
    config->radio_browser_station_limit = get_config_value_int("radio_browser_station_limit", 50);
    config->radio_browser_category_limit = get_config_value_int("radio_browser_category_limit", 50);
    config->radio_browser_language_limit = get_config_value_int("radio_browser_language_limit", 50);
}

static void update_time_item(time_t timer) {
    char buffer[25];

    struct tm current_tm_info;
    localtime_r(&timer, &current_tm_info);
    strftime(buffer, 25, app->time_menu_item_format, &current_tm_info);
    menu_item_set_label(app->time_item, buffer);
    log_debug(MAIN_CTX, "free (buffer))\n");
    log_debug(MAIN_CTX, "buffer = %p\n", buffer);
}

static int weather_listener(weather *weather) {
    if (weather) {
        app->wthr.temp = weather->temp;
        if (app->wthr.weather_icon) {
            free(app->wthr.weather_icon);
        }
        app->wthr.weather_icon = my_copystr((const char *) weather->weather_icon);
        log_config(MAIN_CTX, "Received new weather: %.0f°C\n", weather->temp);
    }

    return 1;
}

static void update_menu_item_label_or_icon(menu_item *item, char *label, char *icon) {
    if (icon) {
        menu_item_set_icon(item, icon);
        menu_item_set_label(item, NULL);
    } else {
        menu_item_set_label(item, label);
        menu_item_set_icon(item, NULL);
    }
}

static void apply_radio_theme(radio_theme *theme) {
    if (app->current_theme == theme) {
        return;
    }

    app->current_theme = theme;
    menu_ctrl_apply_theme(app->ctrl, theme->menu_theme);

    if (theme->info_bg_image_path) {
        menu_set_bg_image(app->info_menu, theme->info_bg_image_path);
    }

    if (theme->volume_bg_image_path) {
        menu_set_bg_image(app->volume_menu, theme->volume_bg_image_path);
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

    menu_set_colors(app->info_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(app->volume_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(app->message_menu, info_default_clr, info_selected_clr, info_scale_clr);
    /* Options menu disabled for now. */

    free(info_default_clr);
    free(info_selected_clr);
    free(info_scale_clr);
}

static void update_metadata_menu_item(menu_item *item, char *value, char *fallback_icon, char *fallback_label) {
    if (value) {
        update_menu_item_label_or_icon(item, value, NULL);
    } else {
        update_menu_item_label_or_icon(item, fallback_label, fallback_icon);
    }
}

static int player_event_mask(player_status status) {
    switch (status) {
    case PLAYER_ACTIVATED:
    case PLAYER_PLAYBACK_PLAYING:
    case PLAYER_PLAYBACK_PAUSED:
    case PLAYER_PLAYBACK_STOPPED:
        return PLAYER_EVENT_STATE;
    case PLAYER_METADATA_CHANGED:
        return PLAYER_EVENT_METADATA;
    case PLAYER_COVER_CHANGED:
        return PLAYER_EVENT_COVER;
    case PLAYER_VOLUME_CHANGED:
        return PLAYER_EVENT_VOLUME;
    default:
        return 0;
    }
}

static radio_theme *get_player_theme(player *p) {
#ifdef SPOTIFY
    if (p == app->spotify_player) {
        return app->spotify_theme;
    }
#endif
#ifdef BLUETOOTH
    if (p == app->bluetooth_player) {
        return app->bluetooth_theme;
    }
#endif
    return NULL;
}

static void update_player_menu_item(player *p) {
    const char *label = player_get_label(p);
    const char *icon = player_get_icon(p);

    if (!label || is_blank((char *) label)) {
        label = player_get_name(p);
    }

    menu_item_set_label(app->player_item, label);
    menu_item_set_icon(app->player_item, NULL);

    menu_item_set_label(app->player_icon_item, NULL);
    if (icon && !is_blank((char *) icon)) {
        if (!access(icon, R_OK)) {
            menu_item_set_icon(app->player_icon_item, icon);
        } else {
            log_warning(MAIN_CTX, "Player icon not readable: %s\n", icon);
            menu_item_set_icon(app->player_icon_item, NULL);
        }
    } else {
        menu_item_set_icon(app->player_icon_item, NULL);
    }
}

static void radio_app_restore_radio_defaults(void) {
    apply_radio_theme(app->default_theme);
    update_player_menu_item(app->radio_player);
    update_menu_item_label_or_icon(app->title_item, "VE 301", NULL);
    update_menu_item_label_or_icon(app->artist_item, "VE 301", NULL);
}

static int process_player_event(player *p, radio_theme *player_theme, int events) {
    if (!p || !events) {
        return p ? player_is_active(p) : 0;
    }

    if (player_is_active(p)) {
        if ((events & PLAYER_EVENT_STATE) && player_active_changed(p)) {
            log_info(MAIN_CTX, "%s is connected.\n", player_get_name(p));

            if (p != app->radio_player) {
                player_playback_stop(app->radio_player);
            }
            if (player_theme) {
                apply_radio_theme(player_theme);
            }
            update_player_menu_item(p);
        }

        if (events & (PLAYER_EVENT_STATE | PLAYER_EVENT_METADATA)) {
            char *title = player_get_title(p);
            char *artist = player_get_artist(p);
            log_config(MAIN_CTX,
                       "Player %s: Metadata changed: Title %s, Artist %s\n",
                       player_get_name(p),
                       title,
                       artist);
            update_metadata_menu_item(app->title_item,
                                          player_get_title(p),
                                          player_get_icon(p),
                                          player_get_label(p));
            update_metadata_menu_item(app->artist_item,
                                          player_get_artist(p),
                                          player_get_icon(p),
                                          player_get_label(p));
        }

        if ((events & (PLAYER_EVENT_STATE | PLAYER_EVENT_COVER))
            && player_get_playback_status(p) == PLAYER_PLAYBACK_PLAYING
            && player_get_cover_image_path(p)) {
            menu_set_bg_image(app->info_menu, player_get_cover_image_path(p));
        }

        if ((events & (PLAYER_EVENT_VOLUME))) {
            int volume = media_player_get_volume();
            radio_app_show_volume_menu(volume);
        }

    } else if ((events & PLAYER_EVENT_STATE) && player_active_changed(p)) {
        log_info(MAIN_CTX, "%s disconnected\n", player_get_name(p));
        if (p != app->radio_player) {
            radio_app_restore_radio_defaults();
        }
    }

    return player_is_active(p);
}

static void process_player_events(void) {
    player_event *event = player_next_event();

    while (event) {
        if (app && event->player) {
            process_player_event(event->player,
                                 get_player_theme(event->player),
                                 player_event_mask(event->status));
        }
        player_event_free(event);
        event = player_next_event();
    }
}

void radio_playback_start(void *data) {
    log_info(MAIN_CTX, "Radio playback started\n");
}

void radio_playback_stop(void *data) {
    log_info(MAIN_CTX, "Radio playback start\n");
}

void start_players() {
    player_start(app->radio_player);
#ifdef SPOTIFY
    if (app->spotify_player) {
        player_start(app->spotify_player);
    }
#endif
#ifdef BLUETOOTH
    if (app->bluetooth_player) {
        player_start(app->bluetooth_player);
    }
#endif
}

void init_players(const char *radio_player_name, const char *radio_player_label) {
    char radio_icon[MAX_CONFIG_LINE_LENGTH];
    config_value_path(radio_icon, "radio_icon", NULL);
    app->radio_player = media_player_new(radio_player_name,
                                         radio_icon,
                                         radio_player_label,
                                         get_config_value_int("check_radio_seconds",
                                                              CHECK_RADIO_SECONDS));
    if (player_get_icon(app->radio_player)) {
        player_set_label(app->radio_player, NULL);
    }


#ifdef SPOTIFY
	if (get_config_value_int("spotify_enabled", 0)) {
		char spotify_host[MAX_CONFIG_LINE_LENGTH];
		config_value(spotify_host, "spotify_host", "localhost");
		char spotify_icon[MAX_CONFIG_LINE_LENGTH];
		config_value_path(spotify_icon, "spotify_icon", NULL);
		int spotify_show_cover = get_config_value_int("spotify_show_cover", 0);
		app->spotify_player = spotify_init(spotify_host, "Spotify",
				spotify_icon, get_config_value_int("check_spotify_seconds",
				CHECK_SPOTIFY_SECONDS), spotify_show_cover);
		if (player_get_icon(app->spotify_player)) {
			player_set_label(app->spotify_player, NULL);
		}

	}

#endif
#ifdef BLUETOOTH
    if (get_config_value_int("bluetooth_enabled", 0)) {
        char bluetooth_icon[MAX_CONFIG_LINE_LENGTH];
        config_value_path(bluetooth_icon, "bluetooth_icon", NULL);
        app->bluetooth_player = bluetooth_init("Bluetooth",
                                               bluetooth_icon,
                                               get_config_value_int("check_bluetooth_seconds",
                                                                    CHECK_BLUETOOTH_SECONDS));
        if (player_get_icon(app->bluetooth_player)) {
            player_set_label(app->bluetooth_player, NULL);
        }
    }

#endif
}

static void update_radio_menu(void) {
    menu_clear(app->radio_menu);

    playlist *internet_radios = media_player_get_internet_radios();
    if (internet_radios != NULL) {
        menu_set_no_items_on_scale(app->radio_menu,
                                   RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                       * menu_ctrl_get_n_o_items_on_scale(app->ctrl));
        unsigned int r = 0;
        for (r = 0; r < internet_radios->n_songs; r++) {
            song *s = internet_radios->songs[r];
            menu_item_new(app->radio_menu, s->title, NULL, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
        }

        menu_item_set_sub_menu(app->radio_menu_item, app->radio_menu);

        free_and_set_null((void **) &internet_radios->name);
        free_and_set_null((void **) &internet_radios->songs);
        free(internet_radios);

    } else {
        menu_item_new(app->radio_menu, "No radio", NULL, NULL, 0, NULL, -1, NULL, NULL, -1);

        log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
    }
}

static void radio_app_open_info_menu(void) {
    if (app && app->info_menu) {
        menu_open(app->info_menu);
    }
}

void vol_label(char *buffer, int volume) {
    if (volume < 0) {
        volume = 0;
    }
    snprintf(buffer, 20, "Volume: %d%%", volume);
}

static void radio_app_show_volume_menu(int volume) {
    char label[20];

    vol_label(label, volume);
    menu_item_set_label(app->volume_menu_item, label);
    menu_item_show(app->volume_menu_item);
}

#ifdef ALSA
static void process_alsa_events(void) {
    int volume = 0;
    int has_event = 0;

    while (alsa_next_volume_event(&volume)) {
        has_event = 1;
    }

    if (has_event) {
        volume = alsa_get_volume();
        log_config(MAIN_CTX, "ALSA volume changed: %d\n", volume);
        radio_app_show_volume_menu(volume);
    }
}
#endif

static int item_action_update_interface_menu(menu_event evt, menu *m, menu_item *item) {
    if (evt == DISPOSE) {
        if (menu_item_get_user_data(item)) {
            network_interface *interface = (network_interface *) menu_item_get_user_data(item);
            free_network_interface(interface);
            menu_item_set_user_data(item, NULL);
        }
    } else if (menu_item_get_sub_menu(item)) {
        menu *sub_menu = menu_item_get_sub_menu(item);

        menu_clear(sub_menu);

        network_interface *interface = (network_interface *) menu_item_get_user_data(item);
        char *ip_address_label = my_catstr("IP\n", interface->ipaddress);
        menu_item_new(
            sub_menu, ip_address_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        free(ip_address_label);

        log_debug(MAIN_CTX, "Getting result from wifi scan\n");
        struct station_info *wifi_scan_result = scan_wifi_station(interface->ifname);
        log_debug(MAIN_CTX, "Result: %p\n", wifi_scan_result);

        if (wifi_scan_result) {
            char *ssid_label = my_catstr("Station\n", wifi_scan_result->ssid);
            log_debug(MAIN_CTX, "%s\n", ssid_label);
            menu_item_new(
                sub_menu, ssid_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
            free(ssid_label);

            char signal_chr[100];
            sprintf(signal_chr, "%d dBm", wifi_scan_result->signal_dbm);
            char *strength_label = my_catstr("Strength\n", signal_chr);
            menu_item_new(sub_menu,
                          strength_label,
                          NULL,
                          NULL,
                          UNKNOWN_OBJECT_TYPE,
                          NULL,
                          -1,
                          NULL,
                          NULL,
                          -1);
            free(strength_label);

            free(wifi_scan_result);
            log_debug(MAIN_CTX, "Done\n");
        }
    }

    return 0;
}

static int item_action_update_network_menu(menu_event evt, menu *m, menu_item *item) {
    if (evt == DISPOSE) {
        return 0;
    }

    menu_item_free_user_data(item);

    menu *sub_menu = menu_item_get_sub_menu(item);
    if (sub_menu) {
        int id = menu_get_max_id(sub_menu);
        while (id >= 0) {
            menu_item_free_user_data(menu_get_item(sub_menu, id--));
        }
        menu_clear(sub_menu);
    }

    network_interfaces *interfaces = get_network_interfaces();

    if (interfaces) {
        menu_item_set_user_data(item, interfaces->interfaces);
        for (int i = 0; i < interfaces->n; i++) {
            network_interface *interface = interfaces->interfaces[i];
            interfaces->interfaces[i] = NULL;
            menu *interface_menu = menu_new(menu_get_ctrl(m), 1, NULL, 0, NULL, NULL, 0);
            menu_set_no_items_on_scale(interface_menu, 3);
            menu_item *interface_item = menu_add_sub_menu(menu_item_get_sub_menu(item),
                                                          interface->ifname,
                                                          interface_menu,
                                                          &item_action_update_interface_menu);
            menu_item_set_user_data(interface_item, interface);
        }

        free(interfaces->interfaces);
        free(interfaces);
    }

    return 0;
}

static void radio_app_open_active_or_nav_menu(void) {
    menu *active_menu = menu_ctrl_get_active(app->ctrl);

    if (active_menu) {
        menu_open(active_menu);
    } else {
        menu_open(app->nav_menu);
    }
}

static void menu_action_activate(menu *m, menu_item *item) {
    if (item) {
        char *label = menu_item_get_label(item);
        log_config(MAIN_CTX, "Action: %s\n", label);
        if (menu_item_get_sub_menu(item)) {
            log_config(MAIN_CTX, "Sub menu: %s\n", label);
            if (item == app->radio_menu_item) {
                update_radio_menu();
            }
        } else if (menu_item_get_user_data(item)
                   && menu_item_get_object_type(item) == OBJ_TYPE_SONG) {
            menu_set_active_id(m, menu_item_get_id(item));
            song *s = (song *) menu_item_get_user_data(item);
            long song_id = player_get_song_id(app->radio_player);
            if (!song_id || (song_id != s->id)) {
                log_info(MAIN_CTX, "Starting radio playback\n");
                if (!player_playback_start(app->radio_player, song_clone(s))) {
                    log_error(MAIN_CTX, "Could not play song %s", s->title);
                }
                const char *station = s->title ? s->title : s->name;
                if (!station) {
                    station = s->artist;
                }
                if (station) {
                    menu_item_set_label(app->artist_item, station);
                }
            } else if (song_id) {
                player_playback_stop(app->radio_player);
            }
        } else if (menu_is_transient(menu_item_get_menu(item))) {
            radio_app_open_active_or_nav_menu();
        }
    } else if (m && menu_is_transient(m)) {
        radio_app_open_active_or_nav_menu();
    }
}

static void mixer_turn_action(menu *m_ptr, int direction) {
    int vol = 0;
    if (app->alsa_enabled) {
#ifdef ALSA
        log_config(MAIN_CTX, "menu_turn_action(%d)\n", direction);
        log_config(MAIN_CTX, "Volume menu item\n");
        vol = alsa_get_volume();
        log_config(MAIN_CTX, "Current volume: %d\n", vol);
        alsa_set_volume(vol + direction * 2);
        vol = alsa_get_volume();
#endif
    } else {
        log_config(MAIN_CTX, "Volume menu item\n");
        vol = media_player_get_volume() + direction * 2;
        log_config(MAIN_CTX, "Current volume: %d\n", vol);
        media_player_set_volume(vol);
        log_config(MAIN_CTX, "New volume: %d\n", vol);
        radio_app_show_volume_menu(vol);
    }
    log_config(MAIN_CTX, "New volume: %d\n", vol);
    radio_app_show_volume_menu(vol);
}

static void init_mixer_menu(const radio_config *config) {
    app->volume_menu = menu_new_root(app->ctrl, 1, config->info_font, config->info_font_size, NULL, 0);
    menu_set_label(app->volume_menu, "Volume");
    menu_set_transient(app->volume_menu, 1);
    menu_set_draw_only_active(app->volume_menu, 1);

    if (config->info_bg_image_path[0]) {
        menu_set_bg_image(app->volume_menu, config->info_bg_image_path);
    }

#ifdef ALSA
    const int volume = alsa_get_volume();
#else
    const int volume = media_player_get_volume();
#endif

    char label[20];
    vol_label(label, volume);

    app->volume_menu_item = menu_item_new(
        app->volume_menu, label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
}

static int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    if (log_level_enabled(MAIN_CTX, IR_LOG_LEVEL_CONFIG)) {
        char *label = item_ptr ? menu_item_get_label(item_ptr) : "";
        log_config(MAIN_CTX, "action(%d, %p, %s)\n", evt, m_ptr, label);
    }

    switch (evt) {
    case ACTIVATE:
        menu_action_activate(m_ptr, item_ptr);
        break;
    case HOLD:
        if (m_ptr == app->nav_menu || m_ptr == app->volume_menu) {
            radio_app_open_info_menu();
        } else if (m_ptr == app->info_menu) {
            /* Options menu disabled for now. */
        } else if (m_ptr && menu_get_parent(m_ptr) && menu_get_parent(m_ptr) != m_ptr) {
            menu_open(menu_get_parent(m_ptr));
        }
        break;
#ifdef ALSA
    case ACTIVATE_1:
        break;
    case HOLD_1:
        break;
    case TURN_LEFT_1:
        mixer_turn_action(m_ptr, -1);
        break;
    case TURN_RIGHT_1:
        mixer_turn_action(m_ptr, 1);
        break;
#else
    case TURN_LEFT_1:
        mixer_turn_action(m_ptr, -1);
        break;
    case TURN_RIGHT_1:
        mixer_turn_action(m_ptr, 1);
        break;
#endif
    case DISPOSE:
        if (menu_item_get_user_data(item_ptr)) {
            const void *obj = menu_item_get_user_data(item_ptr);
            switch (menu_item_get_object_type(item_ptr)) {
            case OBJ_TYPE_DIRECTORY:
            case OBJ_TYPE_MIXER:
                log_debug(MAIN_CTX, "free(%p)\n", obj);
                free((void *) obj);
                menu_item_set_user_data(item_ptr, NULL);
                break;
            case OBJ_TYPE_SONG:
                song_free((song *) obj);
                menu_item_set_user_data(item_ptr, NULL);
                break;
            case OBJ_TYPE_ALBUM:
            case OBJ_TYPE_ARTIST:
                menu_clear(menu_item_get_sub_menu(item_ptr));
                break;
            }
        }
        break;
    default:
        break;
    }

    radio_app_touch_activity();

    return 0;
}

static void radio_app_rotate_info_menu(void) {
    menu *root = app->info_menu;
    int start_id;
    int current_id;
    int max_id;

    if (!root) {
        return;
    }

    start_id = menu_get_current_id(root);
    current_id = start_id;
    max_id = menu_get_max_id(root);

    do {
        menu_item *item;
        char *label;

        if (++current_id > max_id) {
            current_id = 0;
        }

        item = menu_get_item(root, current_id);
        if (menu_item_get_icon(item)) {
            menu_item_warp_to(item);
            return;
        }

        label = menu_item_get_label(item);
        if (label && !is_blank(label)) {
            menu_item_warp_to(item);
            return;
        }
    } while (current_id != start_id);
}

#if 0
static int stop_song_action(menu_event evt, menu *m, menu_item *item) {
    player_playback_stop(app->radio_player);
    return 1;
}

void init_options_menu() {
    app->options_menu = menu_new(app->ctrl, 1, NULL, -1, NULL, NULL, -1);
    menu_set_transient(app->options_menu, 1);
    menu_item_new(app->options_menu,
                  "Stop",
                  NULL,
                  NULL,
                  UNKNOWN_OBJECT_TYPE,
                  NULL,
                  -1,
                  &stop_song_action,
                  NULL,
                  -1);
}
#endif

static void init_info_menu(const radio_config *config) {
    app->info_menu = menu_new_root(app->ctrl, 1, config->info_font, config->info_font_size, NULL, 0);
    menu_set_label(app->info_menu, "Infos");
    menu_set_transient(app->info_menu, 1);


    app->player_icon_item = menu_item_new(app->info_menu,
                                          NULL,
                                          NULL,
                                          NULL,
                                          UNKNOWN_OBJECT_TYPE,
                                          config->info_font,
                                          config->info_font_size,
                                          NULL,
                                          NULL,
                                          -1);
    app->player_item = menu_item_new(app->info_menu,
                                     "VE 301",
                                     NULL,
                                     NULL,
                                     UNKNOWN_OBJECT_TYPE,
                                     config->info_font,
                                     config->info_font_size,
                                     NULL,
                                     NULL,
                                     -1);
    update_player_menu_item(app->radio_player);
    app->title_item = menu_item_new(app->info_menu,
                                    "VE 301",
                                    NULL,
                                    NULL,
                                    UNKNOWN_OBJECT_TYPE,
                                    config->info_font,
                                    config->info_font_size,
                                    NULL,
                                    NULL,
                                    -1);
    app->artist_item = menu_item_new(app->info_menu,
                                     "VE 301",
                                     NULL,
                                     NULL,
                                     UNKNOWN_OBJECT_TYPE,
                                     config->info_font,
                                     config->info_font_size,
                                     NULL,
                                     NULL,
                                     -1);

    time_t timer;
    time(&timer);
    app->time_item = menu_item_new(app->info_menu,
                                   NULL,
                                   NULL,
                                   NULL,
                                   OBJ_TYPE_TIME,
                                   config->info_font,
                                   config->time_font_size,
                                   NULL,
                                   config->info_font,
                                   0.6 * config->time_font_size);
    update_time_item(timer);

    if (strlen(config->weather_api_key) > 0 && strlen(config->weather_location) > 0) {
        init_weather(120, config->weather_api_key, config->weather_location, config->weather_units);
        app->weather_item = menu_item_new(app->info_menu,
                                          " ",
                                          NULL,
                                          NULL,
                                          UNKNOWN_OBJECT_TYPE,
                                          config->weather_font,
                                          config->weather_font_size,
                                          NULL,
                                          config->font,
                                          config->font_size);
        app->temperature_item = menu_item_new(app->info_menu,
                                              "Weather",
                                              NULL,
                                              NULL,
                                              UNKNOWN_OBJECT_TYPE,
                                              config->info_font,
                                              config->temp_font_size,
                                              NULL,
                                              config->font,
                                              config->font_size);
        start_weather_thread(&weather_listener);

    } else {
        app->weather_item = NULL;
    }

    /* init_options_menu(); */
}

static void update_info_menu(const time_t timer) {
    log_config(MAIN_CTX, "Updating info menu\n");
    app->info_menu_t = timer;
    if (!app->current_menu || !menu_is_sticky(app->current_menu)) {
        if (app->internet_available) {
            char *title_item_label = menu_item_get_label(app->title_item);
            if (!strcmp(title_item_label, "No internet")) {
                menu_item_set_label(app->title_item, "VE 301");
            }

            radio_app_rotate_info_menu();

        } else {
            char *title_item_label = menu_item_get_label(app->title_item);
            if (strcmp(title_item_label, "No internet")) {
                menu_item_set_label(app->title_item, "No internet");
            }
            menu_item_warp_to(app->title_item);
        }
    }
}

int menu_call_back(menu_ctrl *ctrl) {
    (void) ctrl;
    long methodTime_s = current_time_millis();

    log_debug(MAIN_CTX, "Start: Callback\n");

    if (menu_ctrl_get_active(app->ctrl) != app->info_menu) {
        app->current_menu = menu_ctrl_get_active(app->ctrl);
    }

    if (check_time_interval(app->check_internet_interval)) {
        log_trace(MAIN_CTX, "Checking internet\n");
        int ia = check_internet();
        if (ia != app->internet_available) {
            app->internet_available = ia;
            log_info(MAIN_CTX,
                     "Internet internet became %s\n",
                     app->internet_available ? "available" : "unavailable");
        }
    }

    process_player_events();
#ifdef ALSA
    process_alsa_events();
#endif

    time_t timer;
    time(&timer);

    if (timer - app->callback_t > CALLBACK_SECONDS) {
        log_config(MAIN_CTX, "Updating time\n");

        app->callback_t = timer;

        if (app->weather_item && app->temperature_item) {
            char temp_str[255];
            sprintf(temp_str, "%.1f°C", ((double) round(10.0 * app->wthr.temp)) / 10);
            menu_item_set_label(app->temperature_item, temp_str);

            if (app->wthr.weather_icon) {
                menu_item_set_label(app->weather_item, app->wthr.weather_icon);
            }
        }

        update_time_item(timer);
    }

    if (timer - app->info_menu_t > app->info_menu_item_seconds) {
        update_info_menu(timer);
    }

    log_debug(MAIN_CTX, "End:  Callback\n");

    long methodTime_e = current_time_millis();

    if (methodTime_e - methodTime_s >= 100) {
        __log_warning(MAIN_CTX, "Time spend: %d\n", methodTime_e - methodTime_s);
    }

    return 1;
}

static void init_navigation_menu(const radio_config *config) {
    app->nav_menu = menu_new_root(app->ctrl, 1, NULL, 0, NULL, 0);
    menu_set_label(app->nav_menu, "Navigation");
    menu_ctrl_set_active(app->ctrl, app->nav_menu);

    app->radio_menu = menu_new(app->ctrl,
                               get_config_value_int("radio_menu_nm_lines", RADIO_MENU_NO_OF_LINES),
                               NULL,
                               0,
                               NULL,
                               NULL,
                               0);
    menu_set_segments_per_item(app->radio_menu,
                               get_config_value_int("radio_menu_segments_per_item",
                                                    RADIO_MENU_SEGMENTS_PER_ITEM));
    app->radio_menu_item = menu_add_sub_menu(app->nav_menu, "Radio", app->radio_menu, NULL);

    if (config->radio_browser_enabled) {
        app->radio_browser_menu = radio_browser_menu_init(app->ctrl,
                                                          &radio_app_touch_activity,
                                                          config->radio_browser_server,
                                                          config->radio_browser_user_agent,
                                                          config->radio_browser_countrycode,
                                                          config->radio_browser_station_limit,
                                                          config->radio_browser_category_limit,
                                                          config->radio_browser_language_limit,
                                                          app->radio_player);
        app->radio_browser_menu_item = menu_add_sub_menu(app->nav_menu,
                                                         "Radio Browser",
                                                         app->radio_browser_menu,
                                                         NULL);
    }

    // app->lib_menu = menu_new(app->ctrl, 1, NULL, 0, NULL, NULL, 0);
    // menu_set_label(app->lib_menu, "Bibliothek");
    // menu_add_sub_menu(app->nav_menu, "Bibliothek", app->lib_menu, NULL);
    // app->album_menu = menu_new(app->ctrl, 1, NULL, 0, NULL, NULL, 0);
    // menu_set_label(app->album_menu, "Alben");
    // menu_add_sub_menu(app->lib_menu, "Alben", app->album_menu, NULL);
    // app->artist_menu = menu_new(app->ctrl, 3, NULL, 0, NULL, NULL, 0);
    // menu_set_label(app->artist_menu, "Artisten");
    // menu_add_sub_menu(app->lib_menu, "Artisten", app->artist_menu, NULL);

    app->system_menu = menu_new(app->ctrl, 1, NULL, 0, NULL, NULL, 0);
    menu_set_label(app->system_menu, "System");
    menu_add_sub_menu(app->nav_menu, "System", app->system_menu, NULL);

    menu *network_menu = menu_new(app->ctrl, 1, NULL, 0, NULL, NULL, 0);
    menu_set_label(network_menu, "Network");
    menu_add_sub_menu(app->system_menu, "Network", network_menu, &item_action_update_network_menu);

}

static struct radio_app *radio_app_new(const radio_config *config) {
    struct radio_app *app = malloc(sizeof(struct radio_app));
    app->info_menu_t = 0;
    app->callback_t = 0;
    app->wthr.temp = 0;
    app->wthr.weather_icon = NULL;
    app->radio_player = NULL;
    app->spotify_player = NULL;
    app->bluetooth_player = NULL;
    app->default_mixer = NULL;
    app->alsa_enabled = config->alsa_enabled;
    return app;
}

static void radio_app_create_menu(const radio_config *config) {
    app->ctrl = menu_ctrl_new(config->w,
                              config->h,
                              config->x_offset,
                              config->y_offset,
                              config->radius_labels,
                              config->draw_scales,
                              config->radius_scales_start,
                              config->radius_scales_end,
                              config->angle_offset,
                              config->font,
                              config->font_size,
                              config->font_size,
                              &menu_action_listener,
                              &menu_call_back);

    app->time_menu_item_format = my_copystr(config->time_menu_item_format);

    app->info_menu_item_seconds = config->info_menu_item_seconds;

    if (config->light_alpha) {
        menu_ctrl_set_light(app->ctrl,
                            config->light_x,
                            config->light_y,
                            config->light_radius,
                            config->light_alpha);
    }

    if (config->light_img[0]) {
        menu_ctrl_set_light_img(app->ctrl, config->light_img, config->light_img_x, config->light_img_y);
    }

    menu_ctrl_set_warp_speed(app->ctrl, config->warp_speed);

    /* Info Menu */
    init_info_menu(config);

    /* Message Menu */
    app->message_menu = menu_new_root(app->ctrl, 1, config->info_font, config->info_font_size, NULL, 0);
    menu_set_label(app->message_menu, "Messages");
    menu_set_segments_per_item(app->message_menu, 1);
    app->message_menu_item = menu_item_new(
        app->message_menu, "", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, -1);

    /* Navigation Menu */
    init_navigation_menu(config);

    /* Mixer Menu */
    init_mixer_menu(config);

    app->default_theme = get_config_theme("Default");
#ifdef BLUETOOTH
    app->bluetooth_theme = get_config_theme("Bluetooth");
#endif
#ifdef SPOTIFY
    app->spotify_theme = get_config_theme("Spotify");
#endif
    apply_radio_theme(app->default_theme);
}

void radio_app_init(const char *name,
                    const char *radio_player_name,
                    const char *radio_player_label,
                    const int verbose_level) {
    base_init(name, stderr, verbose_level);
    radio_config config;
    read_radio_config(&config);
    app = radio_app_new(&config);
    app->check_internet_interval = time_check_interval_new(CHECK_INTERNET_SECONDS);
    check_internet();
#ifdef ALSA
    if (config.alsa_enabled) {
        app->default_mixer = my_copystr(config.alsa_mixer_name);
        if (!alsa_init(config.mixer_device, app->default_mixer)) {
            log_warning(MAIN_CTX,
                        "Could not initialize alsa mixer %s on device %s\n",
                        app->default_mixer,
                        config.mixer_device);
        }
    }
#endif
    init_players(radio_player_name, radio_player_label);
    radio_app_create_menu(&config);
    start_players();
}

void radio_app_loop() {
    menu_ctrl_loop(app->ctrl);
}

void radio_app_close() {
    menu_item_set_label(app->message_menu_item, "Bye");
    menu_item_warp_to(app->message_menu_item);

    log_info(MAIN_CTX, "Closing all\n");
    log_info(MAIN_CTX, "Stopping weather thread\n");
    stop_weather_thread();
    log_info(MAIN_CTX, "Weather thread stopped\n");
#ifdef BLUETOOTH
    if (app->bluetooth_player) {
        log_info(MAIN_CTX, "Stopping bluetooth\n");
        player_stop(app->bluetooth_player);
        log_info(MAIN_CTX, "Bluetooth stopped\n");
    }
#endif
#ifdef SPOTIFY
    if (app->spotify_player) {
        log_info(MAIN_CTX, "Stopping spotify\n");
        player_stop(app->spotify_player);
        log_info(MAIN_CTX, "Spotify stopped\n");
    }
#endif
    if (app->default_theme) {
        free_theme(app->default_theme);
        app->default_theme = NULL;
    }
#ifdef BLUETOOTH
    if (app->bluetooth_theme) {
        free_theme(app->bluetooth_theme);
        app->bluetooth_theme = NULL;
    }
#endif
#ifdef SPOTIFY
    if (app->spotify_theme) {
        free_theme(app->spotify_theme);
        app->spotify_theme = NULL;
    }
#endif
        player_event *event = player_next_event();
        while (event) {
            player_event_free(event);
            event = player_next_event();
        }
    log_info(MAIN_CTX, "Stopping audio\n");
    player_stop(app->radio_player);
    log_info(MAIN_CTX, "Audio stopped\n");
    radio_browser_menu_close();
    menu_ctrl_free(app->ctrl);
    if (app->radio_player) {
        player_free(app->radio_player);
        app->radio_player = NULL;
    }
    if (app->check_internet_interval) {
        time_check_interval_free(app->check_internet_interval);
        app->check_internet_interval = NULL;
    }
#ifdef ALSA
    alsa_close();
    if (app->default_mixer) {
        free(app->default_mixer);
        app->default_mixer = NULL;
    }
#endif
    if (app->time_menu_item_format) {
        free(app->time_menu_item_format);
        app->time_menu_item_format = NULL;
    }
    if (app->wthr.weather_icon) {
        free(app->wthr.weather_icon);
        app->wthr.weather_icon = NULL;
    }
    base_close();
    free(app);
    app = NULL;
}
