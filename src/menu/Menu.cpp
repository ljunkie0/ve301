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

