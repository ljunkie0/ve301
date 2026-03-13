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
#ifndef MENUCTRL_H
#define MENUCTRL_H
#include "Menu.h"
#include "menu_ctrl_priv.h"
#include <stdint.h>

class MenuCtrl {

private:
    menu_ctrl *ctrl;
    friend class Menu;
public:
    MenuCtrl(int w,
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
	  menu_callback *callBack);
    ~MenuCtrl();
    void dispose();
    void loop();
    void setRadii(int radiusLabels, int radiusScalesStart, int radiusScalesEnd);
    int applyTheme(theme *theme);
    int setBackgroundColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setDefaultColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setActiveColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setSelectedColor(u_int8_t r, u_int8_t g, u_int8_t b);
    void setLight(double x, double y, double radius, double alpha);
    void setLightImage(char *path, int x, int y);
    void enableFontBumpmap();
    void disableFontBumpmap();
};

#endif // MENUCTRL_H
