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

#define CALLBACK_SECONDS 5

static void radio_app_open_active_or_nav_menu(
    void) {
    menu *active_menu = menu_ctrl_get_active(app->ctrl);

    if (active_menu) {
        menu_open(active_menu);
    } else {
        menu_open(app->nav_menu);
    }
}

static void menu_action_activate(
    menu *m, menu_item *item) {
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

int menu_action_listener(
    menu_event evt, menu *m_ptr, menu_item *item_ptr) {
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

    if (evt != TURN_LEFT_1 && evt != TURN_RIGHT_1) {
        radio_app_touch_activity(-1);
    }

    return 0;
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

    if (timer - app->info_menu_t >= app->info_menu_item_seconds) {
        update_info_menu(timer);
    }

    log_debug(MAIN_CTX, "End:  Callback\n");

    long methodTime_e = current_time_millis();

    if (methodTime_e - methodTime_s >= 100) {
        __log_warning(MAIN_CTX, "Time spend: %d\n", methodTime_e - methodTime_s);
    }

    return 1;
}
