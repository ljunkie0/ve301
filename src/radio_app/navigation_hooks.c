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

#include "navigation.h"
#include "../radio_browser/menu.h"
#include "../podcast/menu.h"

#include <stddef.h>

static radio_app_navigation_hook *navigation_hooks[] = {
    &radio_browser_attach_navigation_menu,
    &podcast_attach_navigation_menu,
};

void radio_app_attach_navigation_hooks(const radio_app_navigation_context *ctx) {
    for (size_t i = 0; i < sizeof(navigation_hooks) / sizeof(navigation_hooks[0]); i++) {
        navigation_hooks[i](ctx);
    }
}
