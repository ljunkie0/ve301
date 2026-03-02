/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "theme.h"
#include "base/config.h"
#include "base/log_contexts.h"
#include "base/logging.h"
#include <stdlib.h>

radio_theme *new_radio_theme(char *info_bg_image_path, char *info_color, char *info_scale_color, char *volume_bg_image_path, theme *menu_theme) {
    radio_theme *theme = malloc(sizeof(radio_theme));
    theme->info_bg_image_path = info_bg_image_path;
    theme->info_color = info_color;
    theme->info_scale_color = info_scale_color;
    theme->volume_bg_image_path = volume_bg_image_path;
    theme->menu_theme = menu_theme;
    return theme;
}

void free_radio_theme(radio_theme *radio_theme) {
    if (radio_theme->volume_bg_image_path != radio_theme->info_bg_image_path) {
        free(radio_theme->volume_bg_image_path);
    }
    free(radio_theme->info_bg_image_path);
    free (radio_theme->info_color);
    free (radio_theme->info_scale_color);
    free(radio_theme);
}

char *get_theme_path_value(char *key, char *dflt, const char *theme_name) {
    char *theme_value = get_config_value_path_group(key, NULL, theme_name);
    if (!theme_value) {
        theme_value = get_config_value_path(key, dflt);
    }
    return theme_value;
}

char *get_theme_value(char *key, char *dflt, const char *theme_name) {
    char *theme_value = get_config_value_group(key, NULL, theme_name);
    if (!theme_value) {
        theme_value = get_config_value(key, dflt);
    }
    return theme_value;
}

radio_theme *get_config_theme(const char *theme_name) {
    theme *menu_theme = malloc (sizeof(theme));
    menu_theme->background_color = get_theme_value("background_color", "#ffffff", theme_name);
    menu_theme->scale_color = get_theme_value("scale_color", "#ff0000", theme_name);
    menu_theme->indicator_color = get_theme_value("indicator_color", "#ff0000", theme_name);
    menu_theme->default_color = get_theme_value("default_color", "#00ff00", theme_name);
    menu_theme->selected_color = get_theme_value("selected_color", "#0000ff", theme_name);
    menu_theme->activated_color = get_theme_value("activated_color", "#00ffff", theme_name);
    menu_theme->bg_image_path = get_theme_path_value("bg_image_path", 0, theme_name);
    menu_theme->font_bumpmap = get_config_value_int_group("font_bumpmap", get_config_value_int("font_bumpmap", 0), theme_name);
    menu_theme->shadow_offset = get_config_value_int_group("shadow_offset",get_config_value_int("shadow_offset",0), theme_name);
    menu_theme->shadow_alpha = get_config_value_int_group("shadow_alpha", get_config_value_int("shadow_alpha", 0), theme_name);
    menu_theme->bg_cp_colors = 0;
    menu_theme->fg_color_palette = NULL;
    menu_theme->fg_cp_colors = 0;
    menu_theme->bg_color_palette = NULL;

    char *info_menu_bg_path = get_theme_path_value("info_bg_image_path", NULL, theme_name);
    char *volume_menu_bg_path = get_config_value_group("volume_bg_image_path", info_menu_bg_path, theme_name);

    char *info_color = get_config_value_group("info_color", NULL, "Default");
    char *info_scale_color = get_config_value_group("info_scale_color", NULL, "Default");

    log_config(MAIN_CTX, "Radio theme %s:\n", theme_name);
    log_config(MAIN_CTX, "Background color:              %s\n", menu_theme->background_color);
    log_config(MAIN_CTX, "Scale color:                   %s\n", menu_theme->scale_color);
    log_config(MAIN_CTX, "Indicator color:               %s\n", menu_theme->indicator_color);
    log_config(MAIN_CTX, "Default color:                 %s\n", menu_theme->default_color);
    log_config(MAIN_CTX, "Selected color:                %s\n", menu_theme->selected_color);
    log_config(MAIN_CTX, "Activated color:               %s\n", menu_theme->activated_color);
    log_config(MAIN_CTX, "Background image:              %s\n", menu_theme->bg_image_path);
    log_config(MAIN_CTX, "Font bumpmap:                  %d\n", menu_theme->font_bumpmap);
    log_config(MAIN_CTX, "Shadow offset:                 %d\n", menu_theme->shadow_offset);
    log_config(MAIN_CTX, "Shadow alpha:                  %d\n", menu_theme->shadow_alpha);
    log_config(MAIN_CTX, "Info menu background image:    %s\n", info_menu_bg_path);
    log_config(MAIN_CTX, "Info color:                    %s\n", info_color);
    log_config(MAIN_CTX, "Info scale color:              %s\n", info_scale_color);
    log_config(MAIN_CTX, "Volume menu background image:  %s\n", volume_menu_bg_path);

    return new_radio_theme(info_menu_bg_path, info_color, info_scale_color, volume_menu_bg_path, menu_theme);
}

void free_theme(radio_theme *rth) {

    theme *th = rth->menu_theme;

    log_debug(MAIN_CTX, "free_theme(%p)\n",th);
    if (th->background_color)
        free (th->background_color);
    if (th->scale_color)
        free (th->scale_color);
    if (th->indicator_color)
        free (th->indicator_color);
    if (th->default_color)
        free (th->default_color);
    if (th->selected_color)
        free (th->selected_color);
    if (th->activated_color)
        free (th->activated_color);
    if (th->bg_image_path)
        free (th->bg_image_path);
    if (th->bg_color_palette)
        free (th->bg_color_palette);
    if (th->fg_color_palette)
        free (th->fg_color_palette);
    free (th);

    free_radio_theme(rth);

}
