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
#ifndef MENUO_H
#define MENUO_H
#include "MenuCtrl.h"
#include "MenuItem.h"

class MenuCtrl;

class Menu {

private:
    MenuCtrl *ctrl;
    menu *handle;
    friend class MenuItem;
public:
    Menu(MenuCtrl *ctrl,
	 int lines,
	 const char *font,
	 int fontSize,
	 item_action *action,
	 char *font2ndLine,
	 int fontSize2ndLine);
    ~Menu();
    int setBackgroundImage(char *bgImagePath);
    int setColors(SDL_Color *defaultColor, SDL_Color *selectedColor, SDL_Color *scaleColor);
    //menu_item *menu_new_sub_menu(menu *m, const char*label, item_action *action);
    //menu_item *menu_add_sub_menu(menu *m, const char*label, menu *sub_menu, item_action *action);
    int open();
    int clear();
    void turnLeft();
    void turnRight();

};

#endif // MENUO_H
