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
#include "Menu.h"

Menu::Menu(MenuCtrl *ctrl,
         int lines,
         const char *font,
         int font_size,
         item_action *action,
         char *font_2nd_line,
         int font_size_2nd_line) {

	this->ctrl = ctrl;
	this->handle = menu_new(ctrl->ctrl,
			lines,
			font,
			font_size,
			action,
			font_2nd_line,
			font_size_2nd_line);

}

int Menu::setBackgroundImage(char *bgImagePath) {
	return menu_set_bg_image(this->handle, bgImagePath);
}

int Menu::setColors(SDL_Color *defaultColor, SDL_Color *selectedColor, SDL_Color *scaleColor) {
	return menu_set_colors(this->handle, defaultColor, selectedColor, scaleColor);
}

menu_item *Menu::newSubMenu(const char *label, item_action *action) {
    return menu_new_sub_menu(this->handle, label, action);
}

menu_item *Menu::addSubMenu(const char *label, Menu *subMenu, item_action *action) {
    return menu_add_sub_menu(this->handle, label, subMenu ? subMenu->handle : NULL, action);
}

int Menu::open() {
	return menu_open(this->handle);
}

int Menu::clear() {
	return menu_clear(this->handle);
}

void Menu::turnLeft() {
	menu_turn_left(this->handle);
}

void Menu::turnRight() {
	menu_turn_right(this->handle);
}

menu_ctrl *Menu::getCtrl() {
    return menu_get_ctrl(this->handle);
}

menu *Menu::getParent() {
    return menu_get_parent(this->handle);
}

void Menu::setActiveId(int id) {
    menu_set_active_id(this->handle, id);
}

char *Menu::getLabel() {
    return menu_get_label(this->handle);
}

int Menu::isTransient() {
    return menu_is_transient(this->handle);
}

void Menu::setTransient(int transient) {
    menu_set_transient(this->handle, transient);
}

int Menu::isSticky() {
    return menu_is_sticky(this->handle);
}

menu_item *Menu::getCurrentItem() {
    return menu_get_current_item(this->handle);
}

int Menu::getCurrentId() {
    return menu_get_current_id(this->handle);
}

int Menu::getMaxId() {
    return menu_get_max_id(this->handle);
}

void Menu::setNoItemsOnScale(int n) {
    menu_set_no_items_on_scale(this->handle, n);
}

void Menu::setRadiusLabels(int radius) {
    menu_set_radius_labels(this->handle, radius);
}

void Menu::setSegmentsPerItem(int segments) {
    menu_set_segments_per_item(this->handle, segments);
}

void Menu::setDrawOnlyActive(int drawOnlyActive) {
    menu_set_draw_only_active(this->handle, drawOnlyActive);
}

void Menu::setLabel(const char *label) {
    menu_set_label(this->handle, label);
}

Menu::~Menu() {
    if (this->handle) {
        menu_free(this->handle);
        this->handle = NULL;
    }
}
