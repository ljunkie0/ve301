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

/**
 * Represents one single character to be rendered
 **/
#include "glyph_obj.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../util/sdl_util.h"
#include <SDL2/SDL2_rotozoom.h>

/**
* For bump mapping
**/
struct normal_vector {
    double x;
    double y;
};

// the number of possible angles. Don't know yet
#define N_ANGLES 180

/******************* glyph_obj ***************************************/
void glyph_obj_free(glyph_obj *obj) {
    log_debug(MENU_CTX, "glyph_obj_free (%p)\n", obj);
    if (obj) {
        if (obj->animated) {
            glyph_obj_animated *animated = (glyph_obj_animated *) obj;
            SDL_Texture *current_overlay = obj->bumpmap_overlay;

            for (int i = 0; i < animated->n_animations; i++) {
                if (animated->textures && animated->textures[i]) {
                    SDL_DestroyTexture(animated->textures[i]);
                }
                if (animated->surfaces && animated->surfaces[i]) {
                    SDL_FreeSurface(animated->surfaces[i]);
                }
                if (animated->colorss) {
                    free(animated->colorss[i]);
                }
                if (animated->normalss) {
                    free(animated->normalss[i]);
                }
                if (animated->bumpmap_overlays && animated->bumpmap_overlays[i]
                    && animated->bumpmap_overlays[i] != current_overlay) {
                    SDL_DestroyTexture(animated->bumpmap_overlays[i]);
                }
            }

            if (current_overlay) {
                SDL_DestroyTexture(current_overlay);
            }

            free_and_set_null((void **) &animated->textures);
            free_and_set_null((void **) &animated->surfaces);
            free_and_set_null((void **) &animated->colorss);
            free_and_set_null((void **) &animated->normalss);
            free_and_set_null((void **) &animated->bumpmap_overlays);
            free_and_set_null((void **) &animated->bumpmap_texturess);
            free_and_set_null((void **) &animated->light_pixelss);
            free_and_set_null((void **) &obj->rot_center);
            free_and_set_null((void **) &obj->dst_rect);
            free(obj);
            return;
        }

        free_and_set_null((void **) &obj->colors);
        free_and_set_null((void **) &obj->normals);
        free_and_set_null((void **) &obj->rot_center);
        free_and_set_null((void **) &obj->dst_rect);

        if (obj->surface) {
            log_debug(MENU_CTX,"SDL_FreeSurface(obj->surface => %p);\n", obj->surface);
            SDL_FreeSurface(obj->surface);
            obj->surface = NULL;
        }

        if (obj->texture) {
            SDL_DestroyTexture(obj->texture);
            obj->texture = NULL;
        }

        if (obj->bumpmap_overlay) {
            SDL_DestroyTexture(obj->bumpmap_overlay);
            obj->bumpmap_overlay = NULL;
        }

        if (obj->bump_map && obj->bumpmap_textures) {
            for (int a = 0; a < N_ANGLES; a++) {
                if (obj->bumpmap_textures[a]) {
                    SDL_DestroyTexture(obj->bumpmap_textures[a]);
                }
            }
            free_and_set_null((void **) &obj->bumpmap_textures);
        }

        free (obj);

    }
}

void glyph_obj_update_cnt_rad(glyph_obj *glyph_o, SDL_Point center, int radius) {

    glyph_o->dst_rect->x = center.x - 0.5 * glyph_o->dst_rect->w;
    glyph_o->dst_rect->y = center.y - radius - 0.5 * glyph_o->dst_rect->h;
    glyph_o->rot_center->x = 0.5 * glyph_o->dst_rect->w;
    glyph_o->rot_center->y = radius + 0.5 * glyph_o->dst_rect->h;

}

void init_bumpmap_data(glyph_obj *glyph_o) {
    glyph_o->normals = calloc(glyph_o->surface->w * glyph_o->surface->h, sizeof(normal_vector));
    glyph_o->colors = calloc(glyph_o->surface->w * glyph_o->surface->h, sizeof(SDL_Color));

    Uint32 *pixels = (Uint32 *) glyph_o->surface->pixels;

    int bpp = glyph_o->surface->format->BytesPerPixel;
    int pitch = glyph_o->surface->pitch / bpp;

    for (int y = 0; y < glyph_o->surface->h; y++) {
        int o = glyph_o->surface->w * y;
        int p = pitch * y;

        for (int x = 0; x < glyph_o->surface->w; x++) {
            SDL_Color color;
            SDL_GetRGBA(pixels[p + x],
                        glyph_o->surface->format,
                        &(color.r),
                        &(color.g),
                        &(color.b),
                        &(color.a));

            glyph_o->colors[o + x] = color;
        }
    }

    for (int y = 0; y < glyph_o->surface->h; y++) {
        int o = glyph_o->surface->w * y;
        int pop = glyph_o->surface->w * (y - 1);
        int pon = glyph_o->surface->w * (y + 1);

        for (int x = 0; x < glyph_o->surface->w; x++) {
            SDL_Color color = glyph_o->colors[o + x];

            double dx = 0.0;
            double dy = 0.0;

            if (color.a) {
                Uint8 pax = x <= 0 ? 0 : glyph_o->colors[o + (x - 1)].a;
                Uint8 pay = y <= 0 ? 0 : glyph_o->colors[pop + x].a;
                Uint8 nax = x < glyph_o->surface->w - 1
                                ? glyph_o->colors[o + (x + 1)].a
                                : 0;
                Uint8 nay = y < glyph_o->surface->h - 1
                                ? glyph_o->colors[pon + x].a
                                : 0;

                dx = (nax - pax);
                dy = (nay - pay);

                if (dx || dy) {
                    double inv_dn = Q_rsqrt(dx * dx + dy * dy);
                    dx = dx * inv_dn;
                    dy = dy * inv_dn;
                } else {
                    dx = 0.0;
                    dy = 0.0;
                }
            }

            glyph_o->normals[o + x] = (normal_vector){dx, dy};
        }
    }

    normal_vector df;
    df.x = 0;
    df.y = 0;
    SDL_Color transparent;
    transparent.r = 0;
    transparent.g = 0;
    transparent.b = 0;
    transparent.a = 0;

    glyph_o->normals[0] = df;
    glyph_o->colors[0] = transparent;

    for (int y = 1; y < glyph_o->surface->h; y++) {
        int p = glyph_o->surface->w * y;
        if (glyph_o->surface->w * glyph_o->surface->h - p <= 0)
            log_error(MENU_CTX,
                      "Pointer %d out of range %d\n",
                      p,
                      glyph_o->surface->w * glyph_o->surface->h);

        glyph_o->normals[p] = df;
        glyph_o->normals[p - 1] = df;
        glyph_o->colors[p] = transparent;
        glyph_o->colors[p - 1] = transparent;
    }

    for (int x = 0; x < glyph_o->surface->w; x++) {
        glyph_o->normals[x] = df;
        glyph_o->normals[glyph_o->surface->w * (glyph_o->surface->h - 1) + x] = df;
        glyph_o->colors[x] = transparent;
        glyph_o->colors[glyph_o->surface->w * (glyph_o->surface->h - 1) + x] = transparent;
    }
}

void glyph_obj_init_surface(glyph_obj *glyph_o,
                            SDL_Renderer *renderer,
                            SDL_Surface *surface,
                            SDL_Point center,
                            int radius,
                            int bump_map) {
    glyph_o->bumpmap_textures = NULL;
    glyph_o->bumpmap_overlay = NULL;
    glyph_o->bump_map = bump_map;

    glyph_o->surface = surface;
    glyph_o->radius = radius;

    glyph_o->current_angle = -2000.0;

    glyph_o->colors = NULL;
    glyph_o->normals = NULL;

    if (bump_map) {
        init_bumpmap_data(glyph_o);
    }

    glyph_o->texture = SDL_CreateTextureFromSurface(renderer, glyph_o->surface);
    if (!glyph_o->texture) {
        log_error(MENU_CTX, "Could not generate texture from surface: %s\n", SDL_GetError());
    }
    SDL_SetTextureBlendMode(glyph_o->texture, SDL_BLENDMODE_BLEND);

    Uint32 format;
    int access;
    if (!glyph_o->dst_rect) {
        glyph_o->dst_rect = malloc(sizeof(SDL_Rect));
    }
    SDL_QueryTexture(glyph_o->texture,
                     &format,
                     &access,
                     &(glyph_o->dst_rect->w),
                     &(glyph_o->dst_rect->h));

    if (!glyph_o->rot_center) {
        glyph_o->rot_center = malloc(sizeof(SDL_Point));
    }
    glyph_obj_update_cnt_rad(glyph_o, center, radius);

    glyph_o->minx = 0;
    glyph_o->maxx = surface->w;
    glyph_o->miny = 0;
    glyph_o->maxy = surface->h;
    glyph_o->advance = 0;
}

glyph_obj *glyph_obj_new_surface(
    SDL_Renderer *renderer, SDL_Surface *surface, SDL_Point center, int radius, int bump_map) {
    glyph_obj *glyph_o = calloc(1, sizeof(glyph_obj));
    glyph_o->animated = 0;

    glyph_obj_init_surface(glyph_o, renderer, surface, center, radius, bump_map);

    return glyph_o;
}

glyph_obj *glyph_obj_new_animated(SDL_Renderer *renderer,
                                  SDL_Surface **surfaces,
                                  int n_surfaces,
                                  SDL_Point center,
                                  int radius,
                                  int bump_map) {
    glyph_obj_animated *glyph_o = calloc(1, sizeof(glyph_obj_animated));
    glyph_o->glyph_obj.animated = 1;
    glyph_o->n_animations = n_surfaces;
    glyph_o->next_animation = 0;
    glyph_o->surfaces = calloc(n_surfaces, sizeof(SDL_Surface *));
    glyph_o->textures = calloc(n_surfaces, sizeof(SDL_Texture *));
    glyph_o->normalss = calloc(n_surfaces, sizeof(normal_vector *));
    glyph_o->colorss = calloc(n_surfaces, sizeof(SDL_Color *));
    glyph_o->bumpmap_overlays = calloc(n_surfaces, sizeof(SDL_Texture *));

    for (int i = 0; i < n_surfaces; i++) {
        glyph_obj_init_surface((glyph_obj *) glyph_o,
                               renderer,
                               surfaces[i],
                               center,
                               radius,
                               bump_map);
        glyph_o->surfaces[i] = glyph_o->glyph_obj.surface;
        glyph_o->textures[i] = glyph_o->glyph_obj.texture;
        glyph_o->normalss[i] = glyph_o->glyph_obj.normals;
        glyph_o->colorss[i] = glyph_o->glyph_obj.colors;
    }

    return (glyph_obj *) glyph_o;
}

glyph_obj *glyph_obj_new(SDL_Renderer *renderer,
                         uint16_t c,
                         TTF_Font *font,
                         SDL_Color fg,
                         SDL_Point center,
                         int radius,
                         int bump_map) {
    SDL_Surface *surface = TTF_RenderGlyph_Blended(font, c, fg);
    if (surface == NULL) {
        log_error(MENU_CTX, "Could not render glyph %c: %s\n", c, TTF_GetError());
        return NULL;
    }

    glyph_obj *glyph_o = glyph_obj_new_surface(renderer, surface, center, radius, bump_map);

    int minx = 0,maxx = 0,miny = 0,maxy = 0,advance = 0;
    TTF_GlyphMetrics(font,c,&minx,&maxx,&miny,&maxy,&advance);
    glyph_o->minx = minx;
    glyph_o->maxx = maxx;
    glyph_o->miny = miny;
    glyph_o->maxy = maxy;
    glyph_o->advance = advance;

    return glyph_o;
}

void glyph_obj_update_bumpmap_texture(SDL_Renderer *renderer, glyph_obj *glyph_o, double center_x, double center_y, int angle, double l_x, double l_y) {

    Uint32 *bumpmap_pixels;
    int pitch;

    if (!glyph_o || !glyph_o->surface) {
        return;
    }

    if (!glyph_o->colors || !glyph_o->normals) {
        init_bumpmap_data(glyph_o);
        if (!glyph_o->colors || !glyph_o->normals) {
            return;
        }
    }

    if (!glyph_o->bumpmap_overlay) {
        glyph_o->bumpmap_overlay = SDL_CreateTexture(renderer, DEFAULT_SDL_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, glyph_o->surface->w, glyph_o->surface->h);
        if (!glyph_o->bumpmap_overlay) {
            log_error(MENU_CTX, "Could not create bumpmap texture: %s\n", SDL_GetError());
            return;
        }
        SDL_SetTextureBlendMode(glyph_o->bumpmap_overlay, SDL_BLENDMODE_BLEND);
    }

    if (SDL_LockTexture(glyph_o->bumpmap_overlay, NULL, (void **) &bumpmap_pixels, &pitch) != 0) {
        log_error(MENU_CTX, "Could not lock bumpmap texture: %s\n", SDL_GetError());
        return;
    }

    double s, c;

    get_sinus_and_cosinus(angle, &c, &s);

    SDL_PixelFormat *format = glyph_o->surface->format;
    Uint32 transparent = 0;

    /*
     * Update shadow offset direction
     */
    double x = glyph_o->dst_rect->x+0.5*glyph_o->dst_rect->w-center_x;
    double y = glyph_o->dst_rect->y+0.5*glyph_o->dst_rect->h-center_y;
    double c_x_rot = c * x - s * y + center_x;
    double c_y_rot = s * x + c * y + center_y;

    double light_x = c_x_rot - l_x;
    double light_y = c_y_rot - l_y;
    double light_d = (double) Q_rsqrt((float)(light_x*light_x + light_y*light_y));

    glyph_o->shadow_dx = light_d * light_x;
    glyph_o->shadow_dy = light_d * light_y;

    /**
     * See below. Usually, the distance to the light source should be taken for each pixel (to be adjusted)
     * in the glyph. For performance, only take the distance from the top left pixel
     */
    double x_rot = c * (glyph_o->dst_rect->x-center_x) - s * (glyph_o->dst_rect->y - center_y) + center_x;
    double y_rot = s * (glyph_o->dst_rect->x-center_x) + c * (glyph_o->dst_rect->y - center_y) + center_y;

    light_x = x_rot - l_x;
    light_y = y_rot - l_y;

    double inv_light_d = Q_rsqrt((float)(light_x*light_x + light_y*light_y));

    light_x = inv_light_d * light_x;
    light_y = inv_light_d * light_y;

    /**
     * <<
     */

    int bumpmap_pitch_px = pitch / (int)sizeof(Uint32);

    for (int y = 0; y < glyph_o->surface->h; y++) {

        int src_o = glyph_o->surface->w * y;
        int dst_o = bumpmap_pitch_px * y;

        for (int x = 0; x < glyph_o->surface->w; x++) {

            SDL_Color color = glyph_o->colors[src_o + x];

            if (color.a > 1) {

                Uint8 r = color.r,g = color.g,b = color.b,a = color.a;
                normal_vector df = glyph_o->normals[src_o + x];

                if (df.x || df.y) {
                    // angle to light
                    double dx_rot = c * df.x - s * df.y;
                    double dy_rot = s * df.x + c * df.y;

                    double light = light_x * dx_rot + light_y * dy_rot;

                    if (light >= 0.0) {
                        r = r + (255.0 - r) * light;
                        g = g + (255.0 - g) * light;
                        b = b + (255.0 - b) * light;
                    } else {
                        r = r * (1.0+light);
                        g = g * (1.0+light);
                        b = b * (1.0+light);
                    }
                }

                /**
                 * Have to switch B and R, i don't know why
                 */
                Uint32 lght = (b << format->Rshift) | (g << format->Gshift) | (r << format->Bshift) | (a << format->Ashift & format->Amask);
                bumpmap_pixels[dst_o + x] = lght;

            } else {
                bumpmap_pixels[dst_o + x] = transparent;
            }
        }

    }

    SDL_UnlockTexture(glyph_o->bumpmap_overlay);

}

void glyph_obj_animation_update(glyph_obj *glyph_o) {
    if (!glyph_o->animated) {
        return;
    }

    glyph_obj_animated *glyph_o_a = (glyph_obj_animated *) glyph_o;
    int next_animation = glyph_o_a->next_animation;
    glyph_o_a->glyph_obj.colors = glyph_o_a->colorss[next_animation];
    glyph_o_a->glyph_obj.normals = glyph_o_a->normalss[next_animation];
    glyph_o_a->glyph_obj.surface = glyph_o_a->surfaces[next_animation];
    glyph_o_a->glyph_obj.texture = glyph_o_a->textures[next_animation];

    if (glyph_o_a->glyph_obj.bumpmap_overlay && next_animation > 0) {
        glyph_o_a->bumpmap_overlays[next_animation - 1] = glyph_o_a->glyph_obj.bumpmap_overlay;
    }

    glyph_o_a->glyph_obj.bumpmap_overlay = glyph_o_a->bumpmap_overlays[next_animation];

    if (++next_animation >= glyph_o_a->n_animations) {
        next_animation = 0;
    }

    glyph_o_a->next_animation = next_animation;
}
