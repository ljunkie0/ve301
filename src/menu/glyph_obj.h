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

#ifndef GLYPH_OBJ_H
#define GLYPH_OBJ_H

#include<SDL2/SDL.h>
#include<SDL2/SDL_ttf.h>

typedef struct normal_vector normal_vector;

typedef struct glyph_obj {
    SDL_Texture *texture;
    SDL_Surface *surface;
    SDL_Texture *bumpmap_overlay;
    SDL_Texture **bumpmap_textures;
    Uint32 *light_pixels;
    SDL_Color *colors;
    normal_vector *normals;
    int pitch;
    int advance;
    int minx;
    int maxx;
    int miny;
    int maxy;
    SDL_Rect *dst_rect;
    SDL_Point *rot_center;
    double radius;
    double current_angle;
    double shadow_dx;
    double shadow_dy;
    int bump_map;
    int animated;
} glyph_obj;

typedef struct glyph_obj_animated {
    glyph_obj glyph_obj;
    int n_animations;
    int next_animation;
    SDL_Texture **textures;
    SDL_Surface **surfaces;
    SDL_Texture **bumpmap_overlays;
    SDL_Texture ***bumpmap_texturess;
    Uint32 **light_pixelss;
    SDL_Color **colorss;
    normal_vector **normalss;
} glyph_obj_animated;

glyph_obj *glyph_obj_new(SDL_Renderer *renderer,
                         uint16_t c,
                         TTF_Font *font,
                         SDL_Color fg,
                         SDL_Point center,
                         int radius,
                         int bump_map);
glyph_obj *glyph_obj_new_surface(SDL_Renderer *renderer,
                                 SDL_Surface *surface,
                                 SDL_Point center,
                                 int radius,
                                 int bump_map);

glyph_obj *glyph_obj_new_animated(SDL_Renderer *renderer,
                                  SDL_Surface **surfaces,
                                  int n_surfaces,
                                  SDL_Point center,
                                  int radius,
                                  int bump_map);

void glyph_obj_animation_update(glyph_obj *glyph_obj);

void glyph_obj_free(glyph_obj *obj);
void glyph_obj_update_cnt_rad(glyph_obj *glyph_o, SDL_Point center, int radius);
void glyph_obj_update_bumpmap_texture(SDL_Renderer *renderer, glyph_obj *glyph_o, double center_x, double center_y, int angle, double l_x, double l_y);
void glyph_obj_animation_update(glyph_obj *glyph_o);

#endif // GLYPH_OBJ_H
