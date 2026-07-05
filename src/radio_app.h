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
                    const char *radio_player_label,
                    const int verbose_level);
void radio_app_loop();
void radio_app_close();

#endif // VE3_1_H
