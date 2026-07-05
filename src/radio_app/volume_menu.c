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

void vol_label(
    char *buffer, int volume) {
    if (volume < 0) {
        volume = 0;
    }
    snprintf(buffer, 20, "Volume: %d%%", volume);
}

void radio_app_show_volume_menu(
    int volume) {
    char label[20];

    vol_label(label, volume);
    menu_item_set_label(app->volume_menu_item, label);
    menu_item_show(app->volume_menu_item);

    radio_app_touch_activity(2);
}

#ifdef ALSA
void process_alsa_events(
    void) {
    if (app->alsa_enabled) {
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
}
#endif

void mixer_turn_action(
    menu *m_ptr, int direction) {
    int vol = 0;
#ifdef ALSA
    if (app->alsa_enabled) {
        log_config(MAIN_CTX, "menu_turn_action(%d)\n", direction);
        log_config(MAIN_CTX, "Volume menu item\n");
        vol = alsa_get_volume();
        log_config(MAIN_CTX, "Current volume: %d\n", vol);
        alsa_set_volume(vol + direction * 2);
        vol = alsa_get_volume();
    } else {
#endif
        log_config(MAIN_CTX, "Volume menu item\n");
        vol = media_player_get_volume() + direction * 2;
        log_config(MAIN_CTX, "Current volume: %d\n", vol);
        media_player_set_volume(vol);
        log_config(MAIN_CTX, "New volume: %d\n", vol);
#ifdef ALSA
    }
#endif
    log_config(MAIN_CTX, "New volume: %d\n", vol);
    radio_app_show_volume_menu(vol);
}

void init_mixer_menu(
    const radio_config *config) {
    app->volume_menu
        = menu_new_root(app->ctrl, 1, config->info_font, config->info_font_size, NULL, 0);
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
