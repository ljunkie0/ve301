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
#ifndef THEME_H
#define THEME_H

#include "menu/menu_ctrl.h"

typedef struct radio_theme {
    theme *menu_theme;
    char *info_bg_image_path;
    char *volume_bg_image_path;
    char *info_color;
    char *info_scale_color;
} radio_theme;

radio_theme *new_radio_theme(char *info_bg_image_path, char *info_color, char *info_scale_color, char *volume_bg_image_path, theme *menu_theme);
void free_radio_theme(radio_theme *radio_theme);
radio_theme *get_config_theme(const char *theme_name);
void free_theme(radio_theme *rth);

#endif // THEME_H
