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

#ifndef VE301_RADIO_APP_CONFIG_H
#define VE301_RADIO_APP_CONFIG_H

#include "../base/config.h"

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
    int podcast_enabled;
    char podcast_feeds_file[MAX_CONFIG_LINE_LENGTH];
    int podcast_episode_limit;
} radio_config;

#endif // VE301_RADIO_APP_CONFIG_H
