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
#ifndef VE3_1_H
#define VE3_1_H

#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define NO_OF_SCALES 60
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
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
#define RADIO_MENU_SEGMENTS_PER_ITEM 1
#define RADIO_MENU_NO_OF_LINES 3
#define RADIO_MENU_ITEMS_ON_SCALE_FACTOR 4

#define CHECK_RADIO_SECONDS 1
#ifdef BLUETOOTH
/**
 * Bluetooth
 */
#define CHECK_BLUETOOTH_SECONDS 1
#endif

#ifdef SPOTIFY
/**
 * Spotify
 */
#define CHECK_SPOTIFY_SECONDS 1
#endif

typedef enum {
    OBJ_TYPE_SONG,
    OBJ_TYPE_PLAYLIST,
    OBJ_TYPE_ALBUM,
    OBJ_TYPE_ARTIST,
    OBJ_TYPE_DIRECTORY,
    OBJ_TYPE_TIME,
    OBJ_TYPE_MIXER,
    OBJ_TYPE_RADIO_BROWSER_LOCAL,
    OBJ_TYPE_RADIO_BROWSER_TAG_ROOT,
    OBJ_TYPE_RADIO_BROWSER_LANGUAGE_ROOT,
    OBJ_TYPE_RADIO_BROWSER_TAG,
    OBJ_TYPE_RADIO_BROWSER_LANGUAGE,
    OBJ_TYPE_RADIO_BROWSER_STATION
} object_type;

void radio_app_init(const char *app_name,
                    const char *radio_player_name,
                    const char *radio_player_label);
void radio_app_loop();
void radio_app_close();

#endif // VE3_1_H
