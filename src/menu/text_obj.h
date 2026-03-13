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
#ifndef TEXT_OBJ_H
#define TEXT_OBJ_H

#include "glyph_obj.h"

/**
* Represents one text (menu item label)
**/
typedef struct text_obj {
    int n_glyphs; // The number of characters
    int n_glyphs_2nd_line; // The number of characters on the seconds line if there is any
    glyph_obj **glyphs_objs;
    glyph_obj **glyphs_objs_2nd_line;
    int width;
    int width_2nd_line;
    int height;
    int height_2nd_line;
} text_obj;

text_obj *text_obj_new(SDL_Renderer *renderer,
                       char *txt,
                       char *icon,
                       TTF_Font *font,
                       TTF_Font *font_2nd_line,
                       SDL_Color fg,
                       SDL_Point center,
                       int radius,
                       int line,
                       int n_lines,
                       int light_x,
                       int light_y,
                       int bump_map);
void text_obj_free(text_obj *obj);
void text_obj_draw(SDL_Renderer *renderer, SDL_Texture *target, text_obj *label, int radius, int center_x, int center_y, double angle, double light_x, double light_y, int font_bumpmap, int shadow_offset, int shadow_alpha);
void text_obj_update_cnt_rad(text_obj *obj, SDL_Point center, int radius, int line, int n_lines);

#endif // TEXT_OBJ_H
