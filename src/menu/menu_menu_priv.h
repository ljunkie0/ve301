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
#ifndef MENU_PRIV_H
#define MENU_PRIV_H

#include "menu_menu.h"
#include <SDL2/SDL_ttf.h>

typedef struct menu {
    int max_id;
    char *label;
    int current_id;
    int active_id;
    int radius_labels;
    int radius_scales_start;
    int radius_scales_end;
    int dirty; /* boolean indicating whether the menu state has changes and must be redrawn */
    int n_o_lines; /* The number of lines to show the menu items */
    int n_o_items_on_scale; /* The number of items on the scale at the same time. Default is taken from menu_ctrl */
    int segments_per_item; /* The number of segments left and right that belong to a menu item. Default is taken from menu_ctrl */
    int sticky; /* ignored in the menu framework, but can be used to indicate that a menu shall not fade after a certain time */
    double segment;
    menu_item **item;
    menu *parent;
    menu_ctrl *ctrl;
    SDL_Texture *bg_image;
    TTF_Font *font;
    TTF_Font *font2;
    SDL_Color *scale_color; /* The color of the scales */
    SDL_Color *default_color; /* The default foregound color, if NULL, the default_color from menu_ctrl is taken */
    SDL_Color *selected_color; /* The default foregound color, if NULL, the default_color from menu_ctrl is taken */
    uint8_t transient;
    uint8_t draw_only_active;
    item_action *action;
    /**
     * User data
    **/
    const void *object;
} menu;

void menu_set_radius(menu *m, int radius_labels, int radius_scales_start, int radius_scales_end);
void menu_rebuild_glyphs(menu *m);
int menu_clear(menu *m);

#endif // MENU_PRIV_H
