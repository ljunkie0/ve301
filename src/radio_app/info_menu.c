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

#include "private.h"

void radio_app_touch_activity(int seconds) {
    time_t timer;
    time(&timer);

    int diff = 0;
    if (seconds > 0) {
        diff = seconds - app->info_menu_item_seconds;
    }

    app->info_menu_t = timer + diff;
    log_debug(MAIN_CTX, "radio_app_touch_activity: app->info_menu_t = %d\n", app->info_menu_t);
}

void update_time_item(
    time_t timer) {
    char buffer[25];

    struct tm current_tm_info;
    localtime_r(&timer, &current_tm_info);
    strftime(buffer, 25, app->time_menu_item_format, &current_tm_info);
    menu_item_set_label(app->time_item, buffer);
    log_debug(MAIN_CTX, "free (buffer))\n");
    log_debug(MAIN_CTX, "buffer = %p\n", buffer);
}

static int weather_listener(
    weather *weather) {
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

void update_menu_item_label_or_icon(
    menu_item *item, char *label, char *icon) {
    if (icon) {
        menu_item_set_icon(item, icon);
        menu_item_set_label(item, NULL);
    } else {
        menu_item_set_label(item, label);
        menu_item_set_icon(item, NULL);
    }
}

void radio_app_open_info_menu(
    void) {
    if (app && app->info_menu) {
        menu_open(app->info_menu);
    }
}

static void radio_app_rotate_info_menu(
    void) {
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

void init_info_menu(
    const radio_config *config) {
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

void update_info_menu(
    const time_t timer) {
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
