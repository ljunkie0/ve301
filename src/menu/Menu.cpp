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

//menu_item *menu_new_sub_menu(menu *m, const char*label, item_action *action);
//menu_item *menu_add_sub_menu(menu *m, const char*label, menu *sub_menu, item_action *action);
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

Menu::~Menu() {
	menu_free(this->handle);
}

