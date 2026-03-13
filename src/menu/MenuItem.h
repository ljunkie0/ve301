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
#ifndef MENUITEM_H
#define MENUITEM_H
#include "menu.h"
#include "MenuCtrl.h"

class Menu;

class MenuItem {

private:
    Menu *m;
    menu_item *handle;
    void init(Menu *m,
         const char *label,
	 const char *iconPath,
         const void *object,
         int objectType,
         const char *font,
         int fontSize,
         item_action *action,
         char *font2ndLine,
         int fontSize2ndLine);
public:
    MenuItem(Menu *m,
	     const char *label,
	     const char *iconPath,
	     const void *object,
	     int objectType,
	     const char *font,
	     int fontSize,
	     item_action *action,
	     char *font2ndLine,
	     int fontSize2ndLine);
    MenuItem(Menu *m,
	     const char *label);
    MenuItem(Menu *m,
	     const char *label,
	     const char *iconPath);
    ~MenuItem();
    void activate();
    void warpTo();
    void show();
    void updateLabel(const char *label);
    void updateIcon(const char *icon);
    void setVisible(int visible);
    int isVisible();

};

#endif // MENUITEM_H
