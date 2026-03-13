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
#include "MenuCtrl.h"

MenuCtrl::MenuCtrl(int w,
          int xOffset,
          int yOffset,
          int radiusLabels,
          int drawScales,
          int radiusScalesStart,
          int radiusScalesEnd,
          double angleOffset,
          const char *font,
          int fontSize,
          int fontSize2,
          item_action *action,
          menu_callback *callBack) {

	this->ctrl = menu_ctrl_new(
			w,
			w,
			xOffset,
			yOffset,
			radiusLabels,
			drawScales,
			radiusScalesStart,
			radiusScalesEnd,
			angleOffset,
			font,
			fontSize,
			fontSize2,
			action,
			callBack);

}

void MenuCtrl::dispose() {
    menu_ctrl_dispose(this->ctrl);
}

void MenuCtrl::loop() {
	menu_ctrl_loop(this->ctrl);
}

void MenuCtrl::setRadii(int radiusLabels, int radiusScalesStart, int radiusScalesEnd) {
	menu_ctrl_set_radii(this->ctrl,radiusLabels,radiusScalesStart,radiusScalesEnd);
}

int MenuCtrl::setBackgroundColor(Uint8 r, Uint8 g, Uint8 b) {
	return menu_ctrl_set_bg_color_rgb(this->ctrl,r,g,b);
}

int MenuCtrl::setDefaultColor(Uint8 r, Uint8 g, Uint8 b) {
	return menu_ctrl_set_default_color_rgb(this->ctrl,r,g,b);
}

int MenuCtrl::setActiveColor(Uint8 r, Uint8 g, Uint8 b) {
	return menu_ctrl_set_active_color_rgb(this->ctrl,r,g,b);
}

int MenuCtrl::setSelectedColor(Uint8 r, Uint8 g, Uint8 b) {
	return menu_ctrl_set_selected_color_rgb(this->ctrl,r,g,b);
}

void MenuCtrl::setLight(double x, double y, double radius, double alpha) {
	menu_ctrl_set_light(this->ctrl,x,y,radius,alpha);
}

void MenuCtrl::setLightImage(char *path, int x, int y) {
	menu_ctrl_set_light_img(this->ctrl,path,x,y);
}

void MenuCtrl::enableFontBumpmap() {
    this->ctrl->font_bumpmap = 1;
}

void MenuCtrl::disableFontBumpmap() {
    this->ctrl->font_bumpmap = 0;
}

MenuCtrl::~MenuCtrl() {
	menu_ctrl_dispose(this->ctrl);
}

