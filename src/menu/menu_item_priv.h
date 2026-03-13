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
#ifndef MENU_ITEM_PRIV_H
#define MENU_ITEM_PRIV_H

#include "menu_item.h"
#include "text_obj.h"
#include <SDL2/SDL_ttf.h>

typedef struct menu_item {
    int id;
    Uint16 *unicode_label;
    Uint16 *unicode_label2;
    char *label;
    int num_label_chars;
    int num_label_chars2;
    int w;
    int h;
    int visible;
    menu *sub_menu;
    TTF_Font *font;
    TTF_Font *font2;
    menu *menu;
    int line; // The line (0 = default, >0 = above, <0 = below)

    text_obj *label_default;
    text_obj *label_active;
    text_obj *label_current;

    item_action *action;
    int w2;
    int h2;
    char *icon;
    /**
     * User data
    **/
    int object_type;
    const void *object;
} menu_item;

void menu_item_update_cnt_rad(menu_item *item, SDL_Point center, int radius);
int menu_item_draw(menu_item *item, menu_item_state st, double angle);
void menu_item_rebuild_glyphs(menu_item *item);
void menu_item_action(menu_event evt, menu_ctrl *ctrl, menu_item *item);

#endif // MENU_ITEM_PRIV_H
