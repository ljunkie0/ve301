/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VE301_RADIO_APP_NAVIGATION_H
#define VE301_RADIO_APP_NAVIGATION_H

#include "config.h"
#include "../audio/player.h"
#include "../menu/menu_ctrl.h"

typedef void radio_app_touch_activity_fn(int seconds);

typedef struct radio_app_navigation_context {
    menu_ctrl *ctrl;
    menu *nav_menu;
    const radio_config *config;
    player *radio_player;
    radio_app_touch_activity_fn *touch_activity;
} radio_app_navigation_context;

typedef void radio_app_navigation_hook(const radio_app_navigation_context *ctx);

void radio_app_attach_navigation_hooks(const radio_app_navigation_context *ctx);

#endif // VE301_RADIO_APP_NAVIGATION_H
