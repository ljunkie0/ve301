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

#define CHECK_RADIO_SECONDS 1
#define CHECK_BLUETOOTH_SECONDS 1
#define CHECK_SPOTIFY_SECONDS 1

#define PLAYER_EVENT_STATE 1
#define PLAYER_EVENT_METADATA 2
#define PLAYER_EVENT_COVER 4
#define PLAYER_EVENT_VOLUME 8

static void update_metadata_menu_item(
    menu_item *item, char *value, char *fallback_icon, char *fallback_label) {
    if (value) {
        update_menu_item_label_or_icon(item, value, NULL);
    } else {
        update_menu_item_label_or_icon(item, fallback_label, fallback_icon);
    }
}

static int player_event_mask(
    player_status status) {
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

static radio_theme *get_player_theme(
    player *p) {
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

void update_player_menu_item(
    player *p) {
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

static void radio_app_restore_radio_defaults(
    void) {
    apply_radio_theme(app->default_theme);
    update_player_menu_item(app->radio_player);
    update_menu_item_label_or_icon(app->title_item, "VE 301", NULL);
    update_menu_item_label_or_icon(app->artist_item, "VE 301", NULL);
}

static int process_player_event(
    player *p, radio_theme *player_theme, int events) {
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

void process_player_events(
    void) {
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

void radio_playback_start(
    void *data) {
    log_info(MAIN_CTX, "Radio playback started\n");
}

void radio_playback_stop(
    void *data) {
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

void init_players(
    const char *radio_player_name, const char *radio_player_label) {
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
        app->spotify_player = spotify_init(spotify_host,
                                           "Spotify",
                                           spotify_icon,
                                           get_config_value_int("check_spotify_seconds",
                                                                CHECK_SPOTIFY_SECONDS),
                                           spotify_show_cover);
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
