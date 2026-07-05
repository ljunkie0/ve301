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
#include "navigation.h"

#define RADIO_MENU_SEGMENTS_PER_ITEM 1
#define RADIO_MENU_NO_OF_LINES 3
#define RADIO_MENU_ITEMS_ON_SCALE_FACTOR 4

void update_radio_menu(
    void) {
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

void init_navigation_menu(
    const radio_config *config) {
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

    radio_app_navigation_context navigation_context = {
        .ctrl = app->ctrl,
        .nav_menu = app->nav_menu,
        .config = config,
        .radio_player = app->radio_player,
        .touch_activity = &radio_app_touch_activity,
    };
    radio_app_attach_navigation_hooks(&navigation_context);

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
