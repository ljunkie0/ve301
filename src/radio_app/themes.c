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

#include "private.h"

void apply_radio_theme(
    radio_theme *theme) {
    if (app->current_theme == theme) {
        return;
    }

    app->current_theme = theme;
    menu_ctrl_apply_theme(app->ctrl, theme->menu_theme);

    if (theme->info_bg_image_path) {
        menu_set_bg_image(app->info_menu, theme->info_bg_image_path);
    }

    if (theme->volume_bg_image_path) {
        menu_set_bg_image(app->volume_menu, theme->volume_bg_image_path);
    }

    SDL_Color *info_default_clr = NULL, *info_selected_clr = NULL;
    if (theme->info_color != NULL) {
        info_default_clr = html_to_color(theme->info_color);
        info_selected_clr = html_to_color(theme->info_color);
    }

    SDL_Color *info_scale_clr = NULL;
    if (theme->info_scale_color != NULL) {
        info_scale_clr = html_to_color(theme->info_scale_color);
    }

    menu_set_colors(app->info_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(app->volume_menu, info_default_clr, info_selected_clr, info_scale_clr);
    menu_set_colors(app->message_menu, info_default_clr, info_selected_clr, info_scale_clr);
    /* Options menu disabled for now. */

    free(info_default_clr);
    free(info_selected_clr);
    free(info_scale_clr);
}
