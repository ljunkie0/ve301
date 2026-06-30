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
#include "MenuItem.h"

MenuItem::MenuItem(
        Menu *m,
        const char *label,
        const char *iconPath,
        const void *object,
        int objectType,
        const char *font,
        int fontSize,
        item_action *action,
        const char *font2ndLine,
        int fontSize2ndLine) {

    init(m,
            label,
            iconPath,
            object,
            objectType,
            font,
            fontSize,
            action,
            font2ndLine,
            fontSize2ndLine);

}

MenuItem::MenuItem(Menu *m,
        const char *label) {
    init( /* m => */ m,
            /* label => */ label,
            /* iconPath => */ (char *) 0,
            /* object => */ (void *) 0,
            /* objectType => */ UNKNOWN_OBJECT_TYPE,
            /* font => */ (const char *) 0,
            /* fontSize => */ -1,
            /* action => */ (item_action *) 0,
            /* font2ndLine => */ (char *) 0,
            /* fontSize2ndLine => */ -1);
}

MenuItem::MenuItem(Menu *m,
        const char *label,
        const char *iconPath) {
    init( /* m => */ m,
            /* label => */ label,
            /* iconPath => */ iconPath,
            /* object => */ (void *) 0,
            /* objectType => */ UNKNOWN_OBJECT_TYPE,
            /* font => */ (const char *) 0,
            /* fontSize => */ -1,
            /* action => */ (item_action *) 0,
            /* font2ndLine => */ (char *) 0,
            /* fontSize2ndLine => */ -1);
}

void MenuItem::init(
        Menu *m,
        const char *label,
        const char *iconPath,
        const void *object,
        int objectType,
        const char *font,
        int fontSize,
        item_action *action,
        const char *font2ndLine,
        int fontSize2ndLine) {
    this->m = m;
    this->handle = menu_item_new(m->handle,
            label,
            iconPath,
            object,
            objectType,
            font,
            fontSize,
            action,
            font2ndLine,
            fontSize2ndLine);
}

MenuItem::~MenuItem() {
    if (this->handle) {
        menu_item_free(this->handle);
        this->handle = NULL;
    }
}

void MenuItem::activate() {
    menu_item_activate(this->handle);
}

void MenuItem::warpTo() {
    menu_item_warp_to(this->handle);
}

void MenuItem::show() {
    menu_item_show(this->handle);
}

void MenuItem::updateLabel(const char *label) {
    menu_item_update_label(this->handle, label);
}

void MenuItem::updateIcon(const char *icon) {
    menu_item_update_icon(this->handle, icon);
}

void MenuItem::setVisible(const int visible) {
    menu_item_set_visible(this->handle, visible);
}

int MenuItem::isVisible() {
    return menu_item_get_visible(this->handle);
}

int MenuItem::isSubMenu() {
    return menu_item_is_sub_menu(this->handle);
}

char *MenuItem::getLabel() {
    return menu_item_get_label(this->handle);
}

char *MenuItem::getIcon() {
    return menu_item_get_icon(this->handle);
}

void MenuItem::setObjectType(int objectType) {
    menu_item_set_object_type(this->handle, objectType);
}

int MenuItem::getObjectType() {
    return menu_item_get_object_type(this->handle);
}

int MenuItem::isObjectType(int objectType) {
    return menu_item_is_object_type(this->handle, objectType);
}

void MenuItem::setSubMenu(Menu *subMenu) {
    menu_item_set_sub_menu(this->handle, subMenu ? subMenu->handle : NULL);
}

menu *MenuItem::getSubMenu() {
    return menu_item_get_sub_menu(this->handle);
}

const void *MenuItem::getUserData() {
    return menu_item_get_user_data(this->handle);
}

void MenuItem::freeUserData() {
    menu_item_free_user_data(this->handle);
}

void MenuItem::setUserData(void *userData) {
    menu_item_set_user_data(this->handle, userData);
}

int MenuItem::getId() {
    return menu_item_get_id(this->handle);
}

menu *MenuItem::getMenu() {
    return menu_item_get_menu(this->handle);
}
