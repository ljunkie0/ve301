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
#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include "menu_menu.h"

typedef enum { ACTIVE, SELECTED, DEFAULT } menu_item_state;

typedef struct menu_item menu_item;
typedef int item_action(menu_event, menu *, menu_item *);

/**
 * Menu Items
 **/
menu_item *menu_item_new(menu *m,
                         const char *label,
                         const char *icon,
                         const void *object,
                         int object_type,
                         const char *font,
                         int font_size,
                         item_action *action,
                         char *font_2nd_line,
                         int font_size_2nd_line);
int menu_item_dispose(menu_item *item);
void menu_item_free(menu_item *item);
int menu_item_is_sub_menu(menu_item *item);
void menu_item_activate(menu_item *item);
void menu_item_warp_to(menu_item *item);
void menu_item_show(menu_item *item);
menu_item *menu_item_update_label(menu_item *item, const char *label);
char *menu_item_get_label(menu_item *item);
char *menu_item_get_icon(menu_item *i);
menu_item *menu_item_update_icon(menu_item *item, const char *icon);
void menu_item_set_visible(menu_item *item, const int visible);
int menu_item_get_visible(menu_item *item);
void menu_item_set_object_type(menu_item *item, int object_type);
int menu_item_get_object_type(menu_item *item);
int menu_item_is_object_type(menu_item *item, int object_type);
void menu_item_set_sub_menu(menu_item *item, menu *sub_menu);
menu *menu_item_get_sub_menu(menu_item *item);
const void *menu_item_get_user_data(menu_item *item);
void menu_item_free_user_data(menu_item *item);
void menu_item_set_user_data(menu_item *item, void *user_data);
int menu_item_get_id(menu_item *item);
menu *menu_item_get_menu(menu_item *item);

#endif // MENU_ITEM_H
