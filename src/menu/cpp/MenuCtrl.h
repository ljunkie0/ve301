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

#include "../menu_ctrl.h"
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
    MenuCtrl(const MenuCtrl &) = delete;
    MenuCtrl &operator=(const MenuCtrl &) = delete;

    void free();
    void loop();
    void setRadii(int radiusLabels, int radiusScalesStart, int radiusScalesEnd);
    int applyTheme(theme *theme);
    int setBackgroundColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setDefaultColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setActiveColor(u_int8_t r, u_int8_t g, u_int8_t b);
    int setSelectedColor(u_int8_t r, u_int8_t g, u_int8_t b);
    void setLight(double x, double y, double radius, double alpha);
    void setLightImage(const char *path, int x, int y);
    void setOffset(int xOffset, int yOffset);
    void setAngleOffset(double a);
    void setWarpSpeed(int warpSpeed);
    void setActive(menu *active);
    int getNItemsOnScale();
    menu *getActive();
    menu *getRoot();
    void *getUserData();
    void setSdlEventCallback(menu_sdl_event_callback *callback);
    void enableFontBumpmap();
    void disableFontBumpmap();
};

#endif // MENUCTRL_H
