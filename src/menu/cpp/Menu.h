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

#include "../menu_menu.h"
#include "MenuCtrl.h"

class MenuItem;

class Menu {

private:
    MenuCtrl *ctrl;
    menu *handle;
    friend class MenuItem;
    friend class MenuCtrl;

public:
    Menu(MenuCtrl *ctrl,
         int lines,
         const char *font,
         int fontSize,
         item_action *action,
         const char *font2ndLine,
         int fontSize2ndLine);
    ~Menu();
    Menu(const Menu &) = delete;
    Menu &operator=(const Menu &) = delete;

    int setBackgroundImage(const char *bgImagePath);
    int setColors(SDL_Color *defaultColor, SDL_Color *selectedColor, SDL_Color *scaleColor);
    menu_item *newSubMenu(const char *label, item_action *action);
    menu_item *addSubMenu(const char *label, Menu *subMenu, item_action *action);
    int open();
    int clear();
    void turnLeft();
    void turnRight();
    menu_ctrl *getCtrl();
    menu *getParent();
    void setActiveId(int id);
    char *getLabel();
    int isTransient();
    void setTransient(int transient);
    int isSticky();
    menu_item *getCurrentItem();
    int getCurrentId();
    int getMaxId();
    void setNoItemsOnScale(int n);
    void setRadiusLabels(int radius);
    void setSegmentsPerItem(int segments);
    void setDrawOnlyActive(int drawOnlyActive);
    void setLabel(const char *label);
};

#endif // MENUO_H
