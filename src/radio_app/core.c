/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "private.h"

#define CHECK_INTERNET_SECONDS 1

struct radio_app *app;

static struct radio_app *radio_app_new(
    const radio_config *config) {
    struct radio_app *app = calloc(1, sizeof(struct radio_app));
    app->info_menu_t = 0;
    app->callback_t = 0;
    app->wthr.temp = 0;
    app->wthr.weather_icon = NULL;
    app->radio_player = NULL;
#ifdef SPOTIFY
    app->spotify_player = NULL;
#endif
#ifdef BLUETOOTH
    app->bluetooth_player = NULL;
#endif
    app->default_mixer = NULL;
    app->alsa_enabled = config->alsa_enabled;
    return app;
}

static void radio_app_create_menu(
    const radio_config *config) {
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
        menu_ctrl_set_light_img(app->ctrl,
                                config->light_img,
                                config->light_img_x,
                                config->light_img_y);
    }

    menu_ctrl_set_warp_speed(app->ctrl, config->warp_speed);

    /* Info Menu */
    init_info_menu(config);

    /* Message Menu */
    app->message_menu
        = menu_new_root(app->ctrl, 1, config->info_font, config->info_font_size, NULL, 0);
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

void radio_app_init(
    const char *name,
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
