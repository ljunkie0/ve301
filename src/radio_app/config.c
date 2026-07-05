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

#define DEFAULT_WINDOW_WIDTH 320
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define DEFAULT_Y_OFFSET 0
#define DEFAULT_LABEL_RADIUS 100
#define DEFAULT_SCALES_RADIUS_START 120
#define DEFAULT_SCALES_RADIUS_END 200
#define DEFAULT_ANGLE_OFFSET 0.0
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_INFO_FONT_SIZE 24
#define INFO_MENU_ITEM_SECONDS 5

void read_radio_config(
    radio_config *config) {
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
    config->mixer_device[0] = '\0';
    config->alsa_mixer_name[0] = '\0';
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
    config->radio_browser_station_font_size
        = get_config_value_int("radio_browser_station_font_size", config->font_size);
    config->podcast_enabled = get_config_value_int("podcast_enabled", 0);
    config_value_path(config->podcast_feeds_file, "podcast_feeds_file", "podcasts");
    config->podcast_episode_limit = get_config_value_int("podcast_episode_limit", 50);
    config->podcast_episode_font_size
        = get_config_value_int("podcast_episode_font_size", config->font_size);
}
