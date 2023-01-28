/*
 * Copyright 2022 LJunkie
 * http://www.???.??
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <execinfo.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_rotozoom.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include "menu.h"
#include "base.h"
#include "sdl_util.h"

#ifdef RASPBERRY
#include"rotaryencoder.h"
#endif
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff
#else
#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#endif

#define get_kerning TTF_GetFontKerningSizeGlyphs
#define DEFAULT_SDL_PIXELFORMAT SDL_PIXELFORMAT_RGBA32

#define MAX_LABEL_LENGTH 25
#define FONT_DEFAULT "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#define BOLD_FONT_DEFAULT "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"

#define Y_OFFSET 35

#define M_2_X_PI 6.28318530718
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132
#endif

#define VISIBLE_ANGLE 72.0

/**
* Menu item state:
* SELECTED: Currently the one under the indicator
* ACTIVE: the radio station e.g. that is currently played
* DEFAULT: all others
**/
typedef enum {
    ACTIVE, SELECTED, DEFAULT
} menu_item_state;

typedef struct double_rect {
    double w,h,x,y;
} double_rect;

/**
* For bump mapping
**/
typedef struct normal_vector {
    double x;
    double y;
} normal_vector;

/**
* Represents one glyph
**/
typedef struct glyph_obj {
    SDL_Texture *texture;
    SDL_Surface *surface;
    SDL_Texture *light;
    Uint32 *light_pixels;
    int pitch;
    SDL_Color *colors;
    normal_vector *normals;
    int advance;
    int minx;
    int maxx;
    int miny;
    int maxy;
    SDL_Rect *dst_rect;
    SDL_Point *rot_center;
    double current_angle;
    double shadow_dx;
    double shadow_dy;
} glyph_obj;

/**
* Represents one text (menu item  label)
**/
typedef struct text_obj {
    int n_glyphs;
    int n_glyphs_2nd_line;
    glyph_obj **glyphs_objs;
    glyph_obj **glyphs_objs_2nd_line;
    int width;
    int width_2nd_line;
    int height;
    int height_2nd_line;
} text_obj;

static double *cosinuses = NULL;
static double *sinuses = NULL;
static double *square_roots = NULL;

static SDL_Color black = { 0, 0, 0, 255 };
static SDL_Color white = { 255, 255, 255, 255 };

static int current_hour = -1;

/* For FPS measuring */
static Uint32 render_start_ticks;

void menu_turn_left(menu *m);
void menu_turn_right(menu *m);
void menu_fade_out(menu *menu_frm, menu *menu_to);
int menu_ctrl_draw_scale(menu_ctrl *ctrl, double xc, double yc, double r, double R, double angle, unsigned char alpha, int lines);
int menu_ctrl_clear(menu_ctrl *ctrl, double angle);
int menu_ctrl_process_events(menu_ctrl *ctrl);
int menu_draw(menu *m, int clear, int render);
int menu_ctrl_apply_light(menu_ctrl *ctrl);

/**
 * Creates a texture that is blended over the background to create
 * the illusion of a light
 * @brief new_light_texture
 * @param renderer The SDL renderer
 * @param w width of the texture
 * @param h height of the texture
 * @param light_x x position of the lightt
 * @param light_y y position of the light
 * @param radius radius of the light
 * @param alpha alpha value of the texture
 * @return The SDL_Texture representing the light
 */
SDL_Texture *new_light_texture(SDL_Renderer *renderer, int w, int h, int light_x, int light_y, int radius, int alpha) {

    SDL_Texture *light_texture = SDL_CreateTexture(renderer,DEFAULT_SDL_PIXELFORMAT,SDL_TEXTUREACCESS_STATIC,w,h);
    SDL_SetTextureBlendMode(light_texture,SDL_BLENDMODE_MUL);
    SDL_PixelFormat *format = SDL_AllocFormat(DEFAULT_SDL_PIXELFORMAT);

    Uint32 pixels[w*h];

    for (int y = 0; y < h; y++) {
        int o = w * y;
        int sq_y = (y - light_y)*(y - light_y);
        for (int x = 0; x < w; x++) {

            double d = SDL_sqrt((x - light_x)*(x - light_x) + sq_y);
            double l = 255.0 - 255.0 * d / radius;
            if (l < 0.0) l = 0.0;
            if (l > 255.0) l = 255.0;

            Uint32 pixel = alpha;
            if (l >= 0.0) {
                pixel = SDL_MapRGBA(format,l,l,l,alpha);
            }
            pixels[x+o] = pixel;

        }
    }

    SDL_FreeFormat(format);
    SDL_UpdateTexture(light_texture,NULL,pixels,4*w);

    return light_texture;
}

SDL_Texture *load_light_texture(SDL_Renderer *renderer, char *path) {
    return IMG_LoadTexture(renderer,path);
}

void glyph_obj_free(glyph_obj *obj) {
    log_debug(MENU_CTX, "free_glyph_obj(%p)\n", obj);
    if (obj) {

        if (obj->colors) {
            free(obj->colors);
        }

        if (obj->normals) {
            free(obj->normals);
        }

        if (obj->surface) {
            SDL_FreeSurface(obj->surface);
        }

        if (obj->texture) {
            SDL_DestroyTexture(obj->texture);
        }

        if (obj->light) {
            SDL_DestroyTexture(obj->light);
        }

        if (obj->rot_center) {
            free(obj->rot_center);
        }

        if (obj->dst_rect) {
            free(obj->dst_rect);
        }

        free(obj);

    }
}

void glyph_obj_update_cnt_rad(glyph_obj *glyph_o, SDL_Point center, int radius, int light_x, int light_y) {

    glyph_o->dst_rect->x = center.x - 0.5 * glyph_o->dst_rect->w;
    glyph_o->dst_rect->y = center.y - radius - 0.5 * glyph_o->dst_rect->h;
    glyph_o->rot_center->x = 0.5 * glyph_o->dst_rect->w;
    glyph_o->rot_center->y = radius + 0.5 * glyph_o->dst_rect->h;

}

glyph_obj *glyph_obj_new(SDL_Renderer *renderer, uint16_t c, TTF_Font *font, SDL_Color fg, SDL_Point center, int radius, int light_x, int light_y) {

    glyph_obj *glyph_o = calloc(1,sizeof(glyph_obj));

    glyph_o->surface = TTF_RenderGlyph_Blended(font,c,fg);
    if (glyph_o->surface == NULL) {
        log_error(MENU_CTX, "Could not render glyph %c: %s\n", c, TTF_GetError());
        return NULL;
    }

    glyph_o->current_angle = -2000.0;

    glyph_o->normals = calloc(glyph_o->surface->w * glyph_o->surface->h , sizeof(normal_vector));
    glyph_o->colors = calloc(glyph_o->surface->w * glyph_o->surface->h , sizeof(SDL_Color));

    Uint32 *pixels = (Uint32 *) glyph_o->surface->pixels;
    for (int y = 1; y < glyph_o->surface->h-1; y++) {
        int o = glyph_o->surface->w * y;
        int op = glyph_o->surface->w * (y-1);
        int on = glyph_o->surface->w * (y+1);
        for (int x = 1; x < glyph_o->surface->w-1; x++) {
            Uint32 pixel = pixels[o + x];
            SDL_Color color;
            SDL_GetRGBA(pixel,glyph_o->surface->format,&(color.r),&(color.g),&(color.b),&(color.a));

            glyph_o->colors[o + x] = color;

            double dx = 0.0;
            double dy = 0.0;

            if (color.a) {

                Uint8 r,g,b;

                Uint32 p_x_pixel = pixels[o + x - 1];
                Uint8 pax;
                SDL_GetRGBA(p_x_pixel,glyph_o->surface->format,&r,&g,&b,&pax);

                Uint32 p_y_pixel = pixels[op + x];
                Uint8 pay;
                SDL_GetRGBA(p_y_pixel,glyph_o->surface->format,&r,&g,&b,&pay);

                Uint32 n_x_pixel = pixels[o + x + 1];
                Uint8 nax;
                SDL_GetRGBA(n_x_pixel,glyph_o->surface->format,&r,&g,&b,&nax);

                Uint32 n_y_pixel = pixels[on + x];
                Uint8 nay;
                SDL_GetRGBA(n_y_pixel,glyph_o->surface->format,&r,&g,&b,&nay);

                dx = 0.5 * (nax - pax);
                dy = 0.5 * (nay - pay);

                if (dx != 0 || dy != 0) {
                    double dn = SDL_sqrt(dx * dx + dy * dy);
                    dx = dx / dn;
                    dy = dy / dn;
                } else {
                    dx = 0.0;
                    dy = 0.0;
                }

            }

            normal_vector df;
            df.x = dx;
            df.y = dy;
            glyph_o->normals[o + x] = df;

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
        glyph_o->normals[glyph_o->surface->w * y] = df;
        glyph_o->normals[glyph_o->surface->w * y - 1] = df;
        glyph_o->colors[glyph_o->surface->w * y] = transparent;
        glyph_o->colors[glyph_o->surface->w * y - 1] = transparent;
    }

    for (int x = 0; x < glyph_o->surface->w; x++) {
        glyph_o->normals[x] = df;
        glyph_o->normals[glyph_o->surface->w * (glyph_o->surface->h-1) + x] = df;
        glyph_o->colors[x] = transparent;
        glyph_o->colors[glyph_o->surface->w * (glyph_o->surface->h-1) + x] = transparent;
    }

    glyph_o->light = SDL_CreateTexture(renderer,DEFAULT_SDL_PIXELFORMAT,SDL_TEXTUREACCESS_STREAMING,glyph_o->surface->w,glyph_o->surface->h);
    /* We lock the texture here to get the pixels
       locking the texture each time the glyph is drawn is too slow
       Not sure if this is correct but it works */
    SDL_LockTexture(glyph_o->light,NULL,(void **) &(glyph_o->light_pixels),&(glyph_o->pitch));
    SDL_SetTextureBlendMode(glyph_o->light,SDL_BLENDMODE_BLEND);

    glyph_o->texture = SDL_CreateTextureFromSurface(renderer,glyph_o->surface);
    if (!glyph_o->texture) {
        log_error(MENU_CTX, "Could not generate texture from surface: %s\n", SDL_GetError());
    }
    SDL_SetTextureBlendMode(glyph_o->texture,SDL_BLENDMODE_BLEND);

    Uint32 format;
    int access;
    SDL_Rect *dst = malloc(sizeof(SDL_Rect));
    SDL_QueryTexture(glyph_o->texture,&format,&access,&(dst->w),&(dst->h));
    glyph_o->dst_rect = dst;

    SDL_Point *rot_center = malloc(sizeof(SDL_Point));
    glyph_o->rot_center = rot_center;

    glyph_obj_update_cnt_rad(glyph_o,center,radius, light_x, light_y);

    int minx = 0,maxx = 0,miny = 0,maxy = 0,advance = 0;
    TTF_GlyphMetrics(font,c,&minx,&maxx,&miny,&maxy,&advance);
    glyph_o->minx = minx;
    glyph_o->maxx = maxx;
    glyph_o->miny = miny;
    glyph_o->maxy = maxy;
    glyph_o->advance = advance;

    return glyph_o;

}

void glyph_obj_update_light(glyph_obj *glyph_o, double center_x, double center_y, double angle, double l_x, double l_y) {

    log_info(MENU_CTX, "Angle: %f\n", angle);

    Uint32 *light_pixels = glyph_o->light_pixels;

    if (cosinuses == NULL) {
        cosinuses = malloc(10000*sizeof(double));
        sinuses = malloc(10000*sizeof(double));
        square_roots = malloc(10000*sizeof(double));
        for (int i = 0; i < 10000; i++) {
            cosinuses[i] = 2.0;
            sinuses[i] = 2.0;
            square_roots[i] = -100000.0;
        }
    }

    int idx = (int) (10.0 * (angle + 360.0));
    double c = cosinuses[idx];
    double s = sinuses[idx];
    if (c > 1.0) {
        double angle_rad = M_PI * angle / 180.0;
        c = cosf(angle_rad);
        cosinuses[idx] = c;
        s = sinf(angle_rad);
        sinuses[idx] = s;
    }

    SDL_PixelFormat *format = glyph_o->surface->format;
    Uint32 transparent = SDL_MapRGBA(format,0,0,0,0);

    /*
     * Update shadow offset direction
     */

    double c_x_rot = c * (glyph_o->dst_rect->x+0.5*glyph_o->dst_rect->w-center_x) - s * (glyph_o->dst_rect->y+0.5*glyph_o->dst_rect->h - center_y) + center_x;
    double c_y_rot = s * (glyph_o->dst_rect->x+0.5*glyph_o->dst_rect->w-center_x) + c * (glyph_o->dst_rect->y+0.5*glyph_o->dst_rect->h - center_y) + center_y;

    double light_x = c_x_rot - l_x;
    double light_y = c_y_rot - l_y;
    double light_d = (double) Q_rsqrt((float)(light_x*light_x + light_y*light_y));
    glyph_o->shadow_dx = light_d * light_x;
    glyph_o->shadow_dy = light_d * light_y;

    /* Normally, the texture would be locked here, but that takes too much time,
     * so the locking is done once and the pixels are stored in the glyph_o
     * That is not correct I guess but it works
     */
    for (int y = 0; y < glyph_o->surface->h; y++) {

        int o = glyph_o->surface->w * y;
        for (int x = 0; x < glyph_o->surface->w; x++) {
            SDL_Color color = glyph_o->colors[o+x];

            if (color.a > 1) {

                normal_vector df = glyph_o->normals[o+x];

                Uint8 r = color.r,g = color.g,b = color.b,a = color.a;

                if (df.x != 0 || df.y != 0) {


                    // distance to light
                    double x_rot = c * (glyph_o->dst_rect->x+x-center_x) - s * (glyph_o->dst_rect->y+y - center_y) + center_x;
                    double y_rot = s * (glyph_o->dst_rect->x+x-center_x) + c * (glyph_o->dst_rect->y+y - center_y) + center_y;

                    double light_x = x_rot - l_x;
                    double light_y = y_rot - l_y;

                    double inv_light_d = Q_rsqrt((float)(light_x*light_x + light_y*light_y));
/*                    float d_2f = (float)(light_x*light_x + light_y*light_y);
                    int d_2i = (int) (10.0 * d_2f);

                    double light_d = square_roots[d_2i];
                    if (light_d == -100000.0) {
                        light_d = (double) SDL_sqrtf(d_2f);
                        square_roots[d_2i] = light_d;
                    }
*/
                    light_x = inv_light_d * light_x;
                    light_y = inv_light_d * light_y;

                    // angle to light
                    double dx_rot = c * df.x - s * df.y;
                    double dy_rot = s * df.x + c * df.y;

                    double light = light_x * dx_rot + light_y * dy_rot;

                    if (light >= 0.0) {
                        r = r + (255.0 - r) * light;
                        g = g + (255.0 - g) * light;
                        b = b + (255.0 - b) * light;
                    } else {
                        r = r * (1+light);
                        g = g * (1+light);
                        b = b * (1+light);
                    }

                }

                /**
                 * Have to switch B and R, i don't know why
                 */
                Uint32 lght = (r << format->Bshift) | (g << format->Gshift) | (b << format->Rshift) | (a << format->Ashift & format->Amask);
                light_pixels[o + x] = lght;

            } else {
                light_pixels[o + x] = transparent;
            }

        }

    }

    SDL_UnlockTexture(glyph_o->light);
}


void text_obj_free(text_obj *obj) {
    if (obj) {
        if (obj->glyphs_objs) {
            for (int g = 0; g < obj->n_glyphs; g++) {
                glyph_obj_free(obj->glyphs_objs[g]);
            }
            free(obj->glyphs_objs);
        }

        if (obj->glyphs_objs_2nd_line) {
            for (int g = 0; g < obj->n_glyphs_2nd_line; g++) {
                glyph_obj_free(obj->glyphs_objs_2nd_line[g]);
            }
            free(obj->glyphs_objs_2nd_line);
        }

        free(obj);

    }
}

void text_obj_update_cnt_rad(text_obj *obj, SDL_Point center, int radius, int line, int n_lines, int light_x, int light_y) {
    if (obj) {

        if (obj->n_glyphs_2nd_line > 0) {
            radius = radius + obj->height * 0.5;
        }

        radius = radius + 0.8 * obj->height * (line + 0.5 * (n_lines-1));

        if (obj->glyphs_objs) {
            for (int g = 0; g < obj->n_glyphs; g++) {
                glyph_obj_update_cnt_rad(obj->glyphs_objs[g], center, radius, light_x, light_y);
            }
        }

        int radius_new = radius - obj->height;

        if (obj->glyphs_objs_2nd_line) {
            for (int g = 0; g < obj->n_glyphs_2nd_line; g++) {
                glyph_obj_update_cnt_rad(obj->glyphs_objs_2nd_line[g], center, radius_new, light_x, light_y);
            }
        }
    }
}


text_obj *text_obj_new(SDL_Renderer *renderer, char *txt, TTF_Font *font, TTF_Font *font_2nd_line, SDL_Color fg, SDL_Point center, int radius, int line, int n_lines, int light_x, int light_y) {

    if (txt) {
        Uint32 unicode_length = MAX_LABEL_LENGTH;
        Uint16 *unicode_text_2nd_line = NULL;
        Uint32 unicode_length_second_line = MAX_LABEL_LENGTH;
        Uint16 *unicode_text = NULL;
        unicode_text = to_unicode(txt,&unicode_length,&unicode_text_2nd_line,&unicode_length_second_line);

        if (!unicode_text) {
            return NULL;
        }

        text_obj *t = malloc(sizeof(text_obj));
        t->n_glyphs = unicode_length;
        t->glyphs_objs = malloc(unicode_length * sizeof(glyph_obj *));
        log_debug(MENU_CTX, "Rendering surface for %s\n",unicode_text);
        SDL_Surface *text_surface = TTF_RenderUNICODE_Blended(font,unicode_text,fg);

        if (text_surface == NULL) {
            log_error(MENU_CTX, "Could not create glyph surface for %s: %s\n", txt, TTF_GetError());
            return NULL;
        }

        t->width = text_surface->w;
        t->height = text_surface->h;

        if (unicode_length_second_line > 0 && unicode_text_2nd_line != NULL) {
            radius = radius + text_surface->h * 0.5;
        }

        radius = radius + 0.8 * text_surface->h * (line + 0.5 * (n_lines-1));

        SDL_FreeSurface(text_surface);

        unsigned int i = 0;
        for (i = 0; i < unicode_length; i++) {
            glyph_obj *glyph_o = glyph_obj_new(renderer,unicode_text[i],font,fg,center,radius, light_x, light_y);
            if (!glyph_o) {
                log_error(MENU_CTX, "Could not create glyph object for %c\n", unicode_text[i]);
                return NULL;
            }
            t->glyphs_objs[i] = glyph_o;
        }

        t->glyphs_objs_2nd_line = NULL;
        t->n_glyphs_2nd_line = unicode_length_second_line;
        if (unicode_length_second_line > 0 && unicode_text_2nd_line != NULL) {
            t->n_glyphs_2nd_line = unicode_length_second_line;
            t->glyphs_objs_2nd_line = malloc(unicode_length_second_line * sizeof(glyph_obj *));

            text_surface = TTF_RenderUNICODE_Blended(font_2nd_line,unicode_text_2nd_line,fg);
            if (text_surface == NULL) {
                log_error(MENU_CTX, "Could not create glyph surface for %s: %s\n", txt, TTF_GetError());
                return NULL;
            }

            t->width_2nd_line = text_surface->w;
            t->height_2nd_line = text_surface->h;
            SDL_FreeSurface(text_surface);

            unsigned int i = 0;
            int radius_new = radius - text_surface->h;
            for (i = 0; i < unicode_length_second_line; i++) {
                t->glyphs_objs_2nd_line[i] = glyph_obj_new(renderer,unicode_text_2nd_line[i],font_2nd_line,fg,center,radius_new, light_x, light_y);
            }
        }

        free(unicode_text);
        if (unicode_text_2nd_line) {
            free(unicode_text_2nd_line);
        }

        return t;

    }

    return NULL;

}


void text_obj_draw(SDL_Renderer *renderer, SDL_Texture *target, text_obj *label, int radius, int center_x, int center_y, int angle, double light_x, double light_y, int font_bumpmap, int shadow_offset, int shadow_alpha) {

    if (radius == 0) {
        return;
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer,target);
    }

    int c;
    double circumference = M_2_X_PI * radius;
    double advance = - 0.5 * label->width;
    for (c = 0; c < label->n_glyphs; c++) {
        glyph_obj *glyph_obj = label->glyphs_objs[c];
        double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/circumference;

        if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {

            if (font_bumpmap) {

                if (a != glyph_obj->current_angle) {
                    glyph_obj_update_light(glyph_obj,center_x,center_y,a, light_x, light_y);
                }

                if (shadow_offset > 0) {

                    SDL_Rect shadow_dst_rec;
                    shadow_dst_rec.w = glyph_obj->dst_rect->w;
                    shadow_dst_rec.h = glyph_obj->dst_rect->h;
                    shadow_dst_rec.x = glyph_obj->dst_rect->x+shadow_offset*glyph_obj->shadow_dx;
                    shadow_dst_rec.y = glyph_obj->dst_rect->y+shadow_offset*glyph_obj->shadow_dy;

                    Uint8 orig_a, orig_r, orig_g, orig_b;
                    SDL_GetTextureAlphaMod(glyph_obj->light, &orig_a);
                    SDL_GetTextureColorMod(glyph_obj->light, &orig_r, &orig_g, &orig_b);
                    SDL_SetTextureAlphaMod(glyph_obj->light,shadow_alpha);
                    SDL_SetTextureColorMod(glyph_obj->light,0,0,0);

                    SDL_RenderCopyEx(renderer,glyph_obj->light,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);

                    SDL_SetTextureAlphaMod(glyph_obj->light,orig_a);
                    SDL_SetTextureColorMod(glyph_obj->light,orig_r, orig_g, orig_b);
                }

                SDL_RenderCopyEx(renderer,glyph_obj->light,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            } else {

                if (shadow_offset > 0) {

                    SDL_Rect shadow_dst_rec;
                    shadow_dst_rec.w = glyph_obj->dst_rect->w;
                    shadow_dst_rec.h = glyph_obj->dst_rect->h;
                    shadow_dst_rec.x = glyph_obj->dst_rect->x+shadow_offset*glyph_obj->shadow_dx;
                    shadow_dst_rec.y = glyph_obj->dst_rect->y+shadow_offset*glyph_obj->shadow_dy;

                    Uint8 orig_a, orig_r, orig_g, orig_b;
                    SDL_GetTextureAlphaMod(glyph_obj->texture, &orig_a);
                    SDL_GetTextureColorMod(glyph_obj->texture, &orig_r, &orig_g, &orig_b);
                    SDL_SetTextureAlphaMod(glyph_obj->texture,shadow_alpha);
                    SDL_SetTextureColorMod(glyph_obj->texture,0,0,0);

                    SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);

                    SDL_SetTextureAlphaMod(glyph_obj->texture,orig_a);
                    SDL_SetTextureColorMod(glyph_obj->texture,orig_r, orig_g, orig_b);
                }

                SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            }
        }

        glyph_obj->current_angle = a;
        advance += glyph_obj->advance;

    }

    if (label->glyphs_objs_2nd_line > 0) {
        circumference = M_2_X_PI * (radius - label->height);
        advance = - 0.5 * label->width_2nd_line;
        for (c = 0; c < label->n_glyphs_2nd_line; c++) {
            glyph_obj *glyph_obj = label->glyphs_objs_2nd_line[c];
            double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/circumference;

            if (a >= -VISIBLE_ANGLE || a <= VISIBLE_ANGLE) {

                if (font_bumpmap) {

                    if (a != glyph_obj->current_angle) {
                        glyph_obj_update_light(glyph_obj,center_x,center_y,a, light_x, light_y);
                    }

                    if (shadow_offset > 0) {

                        SDL_Rect shadow_dst_rec;
                        shadow_dst_rec.w = glyph_obj->dst_rect->w;
                        shadow_dst_rec.h = glyph_obj->dst_rect->h;
                        shadow_dst_rec.x = glyph_obj->dst_rect->x+shadow_offset*glyph_obj->shadow_dx;
                        shadow_dst_rec.y = glyph_obj->dst_rect->y+shadow_offset*glyph_obj->shadow_dy;

                        Uint8 orig_a, orig_r, orig_g, orig_b;
                        SDL_GetTextureAlphaMod(glyph_obj->light, &orig_a);
                        SDL_GetTextureColorMod(glyph_obj->light, &orig_r, &orig_g, &orig_b);
                        SDL_SetTextureAlphaMod(glyph_obj->light,shadow_alpha);
                        SDL_SetTextureColorMod(glyph_obj->light,0,0,0);

                        SDL_RenderCopyEx(renderer,glyph_obj->light,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);

                        SDL_SetTextureAlphaMod(glyph_obj->light,orig_a);
                        SDL_SetTextureColorMod(glyph_obj->light,orig_r, orig_g, orig_b);

                    }
                    SDL_RenderCopyEx(renderer,glyph_obj->light,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);
                } else {
                    if (shadow_offset > 0) {

                        SDL_Rect shadow_dst_rec;
                        shadow_dst_rec.w = glyph_obj->dst_rect->w;
                        shadow_dst_rec.h = glyph_obj->dst_rect->h;
                        shadow_dst_rec.x = glyph_obj->dst_rect->x+shadow_offset*glyph_obj->shadow_dx;
                        shadow_dst_rec.y = glyph_obj->dst_rect->y+shadow_offset*glyph_obj->shadow_dy;

                        Uint8 orig_a, orig_r, orig_g, orig_b;
                        SDL_GetTextureAlphaMod(glyph_obj->texture, &orig_a);
                        SDL_GetTextureColorMod(glyph_obj->texture, &orig_r, &orig_g, &orig_b);
                        SDL_SetTextureAlphaMod(glyph_obj->texture,shadow_alpha);
                        SDL_SetTextureColorMod(glyph_obj->texture,0,0,0);

                        SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);

                        SDL_SetTextureAlphaMod(glyph_obj->texture,orig_a);
                        SDL_SetTextureColorMod(glyph_obj->texture,orig_r, orig_g, orig_b);
                    }
                    SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);
                }
            }
            glyph_obj->current_angle = a;
            advance += glyph_obj->advance;
        }
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer,NULL);
        SDL_RenderCopy(renderer,target,NULL,NULL);
    }

}

void menu_item_update_cnt_rad(menu_item *item, SDL_Point center, int radius) {

    int lines = 1;
    if (item->num_label_chars2 > 0) {
        lines = 2;
    }

    int line;
    for (line = 0; line < lines; line++) {

        text_obj_update_cnt_rad(item->label_default, center, radius, item->line, item->menu->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
        text_obj_update_cnt_rad(item->label_active, center, radius, item->line, item->menu->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
        text_obj_update_cnt_rad(item->label_current, center, radius, item->line, item->menu->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);

    }

}

int menu_item_draw(menu_item *item, menu_item_state st, double angle) {

    int lines = 1;
    if (item->num_label_chars2 > 0) {
        lines = 2;
    }

    int line;
    for (line = 0; line < lines; line++) {

        text_obj *label = item->label_default;
        if (st == ACTIVE) {
            label = item->label_active;
        } else if (st == SELECTED) {
            label = item->label_current;
        }

        text_obj_draw(item->menu->ctrl->renderer,NULL,label,item->menu->radius_labels,item->menu->ctrl->center.x,item->menu->ctrl->center.y,angle,item->menu->ctrl->light_x,item->menu->ctrl->light_y, item->menu->ctrl->font_bumpmap, item->menu->ctrl->shadow_offset, item->menu->ctrl->shadow_alpha);
    }

    return 0;

}

const void *menu_item_get_object(menu_item *item) {
    return item->object;
}

void menu_item_set_object(menu_item *item, void *object) {
    item->object = object;
}

Uint16 *menu_item_get_label(menu_item *i) {
    return i->unicode_label;
}

menu *menu_item_get_sub_menu(menu_item *item) {
    return item->sub_menu;
}

menu *menu_item_get_menu(menu_item *item) {
    return item->menu;
}

int menu_item_is_sub_menu(menu_item *item) {
    return item->sub_menu != 0;
}

void menu_item_set_sub_menu(menu_item *item, menu *sub_menu) {
    item->sub_menu = sub_menu;
}

int menu_item_get_object_type(menu_item *item) {
    return item->object_type;
}

void menu_item_set_object_type(menu_item *item, int object_type) {
    item->object_type = object_type;
}

int menu_item_get_id(menu_item *item) {
    return item->id;
}

void menu_item_rebuild_glyphs(menu_item *item) {

    menu *m = item->menu;

    if (item->label_default) {
        text_obj_free(item->label_default);
        item->label_default = NULL;
    }

    if (item->label_current) {
        text_obj_free(item->label_current);
        item->label_current = NULL;
    }

    if (item->label_active) {
        text_obj_free(item->label_active);
        item->label_active = NULL;
    }

    TTF_Font *font = item->font;
    if (!font) {
        font = m->ctrl->font;
    }

    TTF_Font *font2 = item->font2;
    if (!font2) {
        font2 = m->ctrl->font2;
    }

    SDL_Renderer *renderer = m->ctrl->renderer;
    item->label_default = text_obj_new(renderer,item->label,font,font2,*m->ctrl->default_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_current = text_obj_new(renderer,item->label,font,font2,*m->ctrl->selected_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_active = text_obj_new(renderer,item->label,font,font2,*m->ctrl->activated_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);

}

int menu_item_set_label(menu_item *item, const char *label) {

    const char *llabel = (label ? label : "");
    if (item->label) {
        free(item->label);
        item->label = NULL;
    }

    item->label = my_copystr(llabel);

    menu_item_rebuild_glyphs(item);

    return 1;
}

menu_item *menu_item_new(menu *m, const char *label, void *object, int object_type,
                         const char *font, int font_size, item_action *action, char *font_2nd_line, int font_size_2nd_line) {

    menu_item *item = malloc(sizeof(menu_item));
    item->unicode_label = NULL;
    item->unicode_label2 = NULL;
    item->utf8_label = NULL;
    item->label = NULL;
    item->object = object;
    item->object_type = object_type;
    item->is_sub_menu = 0;
    item->menu = m;
    item->action = action;
    m->max_id++;
    item->id = m->max_id;

    item->line = (item->id % m->n_o_lines) + 1 - m->n_o_lines;

    if (m->item) {
        m->item = realloc(m->item, (Uint32) (m->max_id + 1) * sizeof(menu_item *));
    } else {
        m->item = malloc((Uint32) (m->max_id + 1) * sizeof(menu_item *));
    }
    m->item[m->max_id] = item;
    item->font = NULL;
    item->font2 = NULL;
    item->glyph_objs = NULL;
    item->glyph_objs2 = NULL;
    item->num_label_chars = 0;
    item->num_label_chars2 = 0;
    item->label_active = NULL;
    item->label_current = NULL;
    item->label_default = NULL;

    /* Initialize fonts */

    if (font && font_size > 0) {
        TTF_Font *dflt_font = TTF_OpenFont(font, font_size);
        if (!dflt_font) {
            log_error(MENU_CTX, "Failed to load font: %s. Trying fixed font\n", SDL_GetError());
            dflt_font = TTF_OpenFont("fixed", font_size);
            if (!dflt_font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
        if (dflt_font) {
            item->font = dflt_font;
        }
    } else {
        item->font = m->ctrl->font;
    }

    if (font_2nd_line) {
        if (font_size_2nd_line <= 0) {
            font_size_2nd_line = font_size;
        }
        TTF_Font *dflt_font = TTF_OpenFont(font_2nd_line, font_size_2nd_line);
        if (!dflt_font) {
            log_error(MENU_CTX, "Failed to load font: %s. Trying fixed font\n", SDL_GetError());
            dflt_font = TTF_OpenFont("fixed", font_size_2nd_line);
            if (!dflt_font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
        if (dflt_font) {
            item->font2 = dflt_font;
        }
    } else {
        item->font2 = m->ctrl->font2;
    }

    menu_item_set_label(item, label);

    return item;

}

int menu_item_dispose(menu_item *item) {
    log_config(MENU_CTX, "Start dispose_menu_item\n");

    menu *m = (menu *) item->menu;

    if (!m) {
        log_error(MENU_CTX, "Item %s has no menu!\n", item->unicode_label);
        return 1;
    }

    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;

    if (ctrl->action) {
        // To dispose the object
        ctrl->action(DISPOSE,m,item);
    }

    if (item->is_sub_menu && item->sub_menu) {
        menu_clear((menu *)item->sub_menu);
        log_config(MENU_CTX, "free(item->sub_menu -> %p)\n", item->sub_menu);
        free(item->sub_menu);
        item->sub_menu = NULL;
    }

    if (item->unicode_label) {
        log_config(MENU_CTX, "free(item->unicode_label -> %p)\n", item->unicode_label);
        free(item->unicode_label);
    }
    log_config(MENU_CTX, "free(item -> %p)\n", item);
    free(item);
    log_config(MENU_CTX, "End dispose_menu_item\n");
    return 0;
}

menu_item *menu_item_next(menu *m, menu_item *item) {
    if (item) {
        if (item->id < m->max_id) {
            return m->item[item->id+1];
        }
    }
    return 0;
}

menu_item *menu_item_update_label(menu_item *item, const char *label) {
    log_config(MENU_CTX, "menu_item_update_label(item -> %p, label -> %s)\n", item, label);
    menu *m = (menu *) item->menu;
    menu_item_set_label(item, label);
    menu_ctrl_draw(m->ctrl);
    return item;
}

void menu_item_activate(menu_item *item) {
    menu *m = (menu *) item->menu;
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    menu_item_warp_to(item);
    m->active_id = item->id;
    menu_ctrl_draw(ctrl);
}

void menu_item_debug(menu_item *item) {
    log_debug(MENU_CTX, "Ptr:   %p\n",item);
    log_debug(MENU_CTX, "Label: %s\n",item->utf8_label);
    log_debug(MENU_CTX, "Id:    %d\n",item->id);
    log_debug(MENU_CTX, "Length: %d\n",item->num_label_chars);
}

void menu_item_show(menu_item *item) {
    menu_item_debug(item);
    menu *m = (menu *) item->menu;
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    if (m != ctrl->current) {
        ctrl->warping = 1;
        ctrl->current = m;
        if (!m->transient) {
            ctrl->active = m;
        }
        m->radius_labels = ctrl->radius_labels;
        m->radius_scales_start = ctrl->radius_scales_start;
        m->radius_scales_end = ctrl->radius_scales_end;
        m->current_id = item->id;
        m->active_id = item->id;
        m->segment = 0;
        menu_ctrl_draw(ctrl);
        ctrl->warping = 0;
    }
}

void menu_item_warp_to(menu_item *item) {

    log_config(MENU_CTX, "menu_item_warp_to: menu_item->menu->ctrl->object = %p->%p->%p->%p\n", item, item->menu, item->menu->ctrl, item->menu->ctrl->object);
    menu *m = (menu *) item->menu;
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    if (m != ctrl->current) {
        menu_fade_out(ctrl->current, m);
        ctrl->current = m;
        if (!m->transient) {
            ctrl->active = m;
        }
    }

    ctrl->warping = 1;

    int dist_right  = (item->id < m->current_id) ? m->current_id - item->id : m->current_id + m->max_id - item->id;
    int dist_left   = (item->id < m->current_id) ? m->max_id - m->current_id + item->id : item->id - m->current_id;

    if (dist_right < dist_left) {
        while (item->id != m->current_id && ctrl->warping) {
            menu_turn_right(m);
            //menu_ctrl_draw(ctrl);
            menu_ctrl_process_events(ctrl);
        }
    } else {
        while (item->id != m->current_id && ctrl->warping) {
            menu_turn_left(m);
            //menu_ctrl_draw(ctrl);
            menu_ctrl_process_events(ctrl);
        }
    }

    // Lock in
    while (m->segment < 0 && ctrl->warping) {
        menu_turn_right(m);
        //menu_ctrl_draw(ctrl);
        menu_ctrl_process_events(ctrl);
    }

    while (m->segment > 0 && ctrl->warping) {
        menu_turn_left(m);
        //menu_ctrl_draw(ctrl);
        menu_ctrl_process_events(ctrl);
    }

    ctrl->warping = 0;
    log_config(MENU_CTX, "menu_item_warp_to: ctrl->object = %p->%p\n", ctrl, ctrl->object);

}

void menu_fade_out(menu *menu_frm, menu *menu_to) {
    menu_ctrl *ctrl = (menu_ctrl *) menu_frm->ctrl;
    ctrl->warping = 1;
    double R_frm = menu_frm ? menu_frm->radius_scales_end : 0.0;
    double r_frm = menu_frm ? menu_frm->radius_labels : 0.0;
    double d_frm = R_frm - r_frm;

    double R_to = r_frm;
    double r_to = 0;

    int s = 5;
    double ds = (double) s;
    double inv_ds = 1 / ds;

    int i = 0;
    for (i = 0; i <= s; i++) {

        double x = (double) i;
        double x_1 = ds - x;

        if (menu_frm) {
            menu_frm->radius_labels = to_int(inv_ds * (x * R_frm + x_1 * r_frm));
            menu_frm->radius_scales_end = to_int(inv_ds * (x * (R_frm + d_frm) + x_1 * R_frm));
            menu_draw(menu_frm, 1, 0);
        }
        menu_to->radius_labels = to_int(inv_ds * (x * r_frm + x_1 * r_to));
        menu_to->radius_scales_end = to_int(inv_ds * (x * R_frm + x_1 * R_to));
        menu_draw(menu_to, 0, 1);
    }
    menu_draw(menu_to, 1, 1);
    if (!menu_to->transient) {
        ctrl->active = menu_to;
    }
}

void menu_fade_in(menu *menu_frm, menu *menu_to) {
    menu_ctrl *ctrl = (menu_ctrl *) menu_frm->ctrl;
    ctrl->warping = 1;
    double R_frm = menu_frm->radius_scales_end;
    double r_frm = menu_frm->radius_labels;
    double d_frm = R_frm - r_frm;

    double r_to = R_frm;
    double R_to = R_frm + d_frm;

    int s = 5;
    double ds = (double) s;
    double inv_ds = 1 / ds;

    int i = 0;
    for (i = 0; i <= s; i++) {

        double x = (double) i;
        double x_1 = ds - x;

        menu_frm->radius_labels = to_int(inv_ds * x_1 * r_frm);
        menu_frm->radius_scales_end = to_int(inv_ds * (x * r_frm + x_1 * R_frm));
        menu_to->radius_labels = to_int(inv_ds * (x * r_frm + x_1 * r_to));
        menu_to->radius_scales_end = to_int(inv_ds * (x * R_frm + x_1 * R_to));
        menu_draw(menu_frm, 1, 0);
        menu_draw(menu_to, 0, 1);
    }
    menu_draw(menu_to, 1, 1);
    if (!menu_to->transient) {
        ctrl->active = menu_to;
    }
}

int menu_open_sub_menu(menu_ctrl *ctrl, menu_item *item) {

    menu *sub_menu = (menu *) item->sub_menu;
    sub_menu->segment = 0;
    menu_fade_out(ctrl->current, sub_menu);
    ctrl->current = sub_menu;
    if (!sub_menu->transient) {
        ctrl->active = ctrl->current;
    }
    return 1;
}

int menu_open(menu *m) {
    menu_ctrl *ctrl = m->ctrl;
    if (ctrl->current != m) {
        m->segment = 0;
        menu_fade_out(ctrl->current, m);
        ctrl->current = m;
        if (!m->transient) {
            ctrl->active = ctrl->current;
        }
    }
    return 1;
}

int menu_get_max_id(menu *m) {
    return m->max_id;
}

menu_item *menu_get_item(menu *m, int id) {
    return m->item[id];
}

int menu_draw(menu *m, int clear, int render) {

    log_debug(MENU_CTX, "START: menu_draw\n");
    render_start_ticks = SDL_GetTicks();

    if (!m) {
        return 0;
    }


    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;

    double xc = ctrl->center.x;
    double yc = ctrl->center.y;

    // Eventually change background color according to time of day
    if (ctrl->bg_color_of_time) {
        time_t timer;
        time(&timer);
        struct tm *tm_info = localtime(&timer);
        int t = tm_info->tm_hour * 60 + tm_info->tm_min;
        if (t != current_hour) {

            double m = (double) t;

            if (ctrl->bg_color_palette) {
                int col_idx = (int) (m * (double) ctrl->bg_cp_colors / 1440.0);
                char *bg_color = ctrl->bg_color_palette[col_idx];
                SDL_Color *bg = html_to_color(bg_color);
                menu_ctrl_set_bg_color_rgb(ctrl, bg->r,bg->g,bg->b);
                free(bg);
            } else {
                double hue = 360.0 * (m-720.0+240.0)/1440.0;

                if (hue < 0.0) {
                    hue = hue + 1.0;
                } else if (hue > 1.0) {
                    hue = hue - 1.0;
                }

                menu_ctrl_set_bg_color_hsv(ctrl, hue, 50.0, 100.0);

                current_hour = t;
            }


        }
    }

    double angle = ctrl->angle_offset + m->segment * 360.0 / (m->n_o_items_on_scale*(2*m->n_o_segments+1));

    if (clear) {
        double bg_angle = ctrl->angle_offset + ctrl->bg_segment * 360.0 / (m->n_o_items_on_scale*(2*m->n_o_segments+1));
        menu_ctrl_clear(ctrl,bg_angle);
    }

    if (ctrl->draw_scales) {
        int no_of_mini_scales = 5;
        char draw_inner_mini_scales = 0;

        double a = angle;
        double angle_step = 360.0 / (no_of_mini_scales * (ctrl->no_of_scales));
        double cos_a;
        double sin_a;
        double fr = m->radius_scales_start;
        double fR = m->radius_scales_end;
        cos_a = cos(a);
        sin_a = sin(a);

        /* Draw the scales */

        for (int s = 0; s < ctrl->no_of_scales; s++) {

            menu_ctrl_draw_scale(ctrl, xc, yc, fr-20, fR, a, 255, 1);

            a += angle_step;

            int sd;
            for (sd = 0; sd < no_of_mini_scales; sd++) {

                menu_ctrl_draw_scale(ctrl,xc,yc,fr,fR,a, 255, 0);

                if (draw_inner_mini_scales) {
                    Sint16 x1 = to_Sint16(xc + (fr + 50) * cos_a);
                    Sint16 y1 = to_Sint16(yc - (fr + 50) * sin_a);
                    Sint16 x2 = to_Sint16(xc + (fr + 70) * cos_a);
                    Sint16 y2 = to_Sint16(yc - (fr + 70) * sin_a);

                    SDL_SetRenderDrawColor(ctrl->renderer, ctrl->scale_color->r,
                                           ctrl->scale_color->g, ctrl->scale_color->b, 100);
                    SDL_RenderDrawLine(ctrl->renderer, x1, y1, x2, y2);
                }

                a += angle_step;

            }

        }


    }

    if (m->max_id >= 0) {

        int i;
        int count_drawn_items = (ctrl->warping && !m->draw_only_active) ? 0.5 * m->n_o_items_on_scale : 0;
        for (i = -count_drawn_items; i <= count_drawn_items; i++) {

            int current_item = m->current_id + i;
            if (current_item < 0) {
                current_item = current_item + m->max_id + 1;
            }

            current_item %= (m->max_id + 1);

            menu_item_state st = DEFAULT;
            if (current_item == m->active_id) {
                st = ACTIVE;
            } else if (i == 0) {
                st = SELECTED;
            }


            double item_angle = angle + 360 * i / m->n_o_items_on_scale;

            if (item_angle > 360) {
                item_angle -= 360;
            }

            if (item_angle < - 360) {
                item_angle += 360;
            }

            menu_item_draw(m->item[current_item],st,item_angle);

        }
    }

    Sint16 xc_Sint16 = to_Sint16(xc);
    SDL_SetRenderDrawColor(ctrl->renderer, ctrl->indicator_color->r,
                           ctrl->indicator_color->g, ctrl->indicator_color->b, ctrl->indicator_alpha);
    SDL_RenderDrawLine(ctrl->renderer, xc_Sint16, 0, xc_Sint16, (Sint16) ctrl->w);
    SDL_RenderDrawLine(ctrl->renderer, xc_Sint16 - 1, 0, xc_Sint16 - 1, (Sint16) ctrl->w);
    SDL_RenderDrawLine(ctrl->renderer, xc_Sint16 + 1, 0, xc_Sint16 + 1, (Sint16) ctrl->w);
    SDL_SetRenderDrawColor(ctrl->renderer, ctrl->indicator_color_light->r,
                           ctrl->indicator_color_light->g, ctrl->indicator_color_light->b, ctrl->indicator_alpha * 180 / 255);
    SDL_RenderDrawLine(ctrl->renderer, xc_Sint16 - 2, 0, xc_Sint16 - 2, (Sint16) ctrl->h);
    SDL_SetRenderDrawColor(ctrl->renderer, ctrl->indicator_color_dark->r,
                           ctrl->indicator_color_dark->g, ctrl->indicator_color_dark->b, ctrl->indicator_alpha * 180 / 255);
    SDL_RenderDrawLine(ctrl->renderer, xc_Sint16 + 2, 0, xc_Sint16 + 2, (Sint16) ctrl->h);

    menu_ctrl_apply_light(ctrl);


    if (render) {
        SDL_RenderPresent(ctrl->renderer);
    }

    Uint32 render_passed_ticks = SDL_GetTicks()-render_start_ticks;
    if (render_passed_ticks > 0) {
        log_debug(MENU_CTX, "Render FPS: %f\n", 1000.0/(double)render_passed_ticks);
    }

    return 0;

}

int menu_clear(menu *m) {
    log_config(MENU_CTX, "Start clear_menu, max_id=%d\n", m->max_id);
    int id = m->max_id;
    while (id >= 0) {
        log_config(MENU_CTX, "%d\n",id);
        menu_item_dispose(m->item[id]);
        m->item[id] = 0;
        id--;
    }

    m->max_id = -1;
    m->active_id = -1;
    m->current_id = 0;

    m->segment = 0;

    log_config(MENU_CTX, "End clear_menu\n");
    return 0;
}

menu *menu_new(menu_ctrl *ctrl) {

    menu *m = malloc(sizeof(menu));

    m->max_id = -1;
    m->active_id = -1;
    m->current_id = 0;
    m->object = 0;

    m->radius_labels = ctrl->radius_labels;
    m->radius_scales_start = ctrl->radius_scales_start;
    m->radius_scales_end = ctrl->radius_scales_end;

    m->parent = m;
    m->item = NULL;
    m->segment = 0;
    m->label = 0;
    m->object = 0;

    m->ctrl = ctrl;

    m->transient = 0;
    m->draw_only_active = 0;
    m->n_o_lines = 1;
    m->n_o_items_on_scale = ctrl->n_o_items_on_scale;
    m->n_o_segments = ctrl->n_o_segments;

    return m;

}

menu *menu_new_root(menu_ctrl *ctrl) {
    menu *m = menu_new(ctrl);
    ctrl->n_roots = ctrl->n_roots + 1;
    ctrl->root = realloc(ctrl->root,ctrl->n_roots * sizeof(menu *));
    ctrl->root[ctrl->n_roots-1] = m;
    return m;
}

void menu_dispose(menu *menu) {
    if (!menu) {
        return;
    }

    menu_clear(menu);

}

menu_item *menu_add_sub_menu(menu *m, const char*label, menu *sub_menu, item_action *action) {

    menu_item *item = menu_item_new(m, label, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;
    item->is_sub_menu = 1;
    sub_menu->parent = m;

    return item;

}

menu_item *menu_new_sub_menu(menu *m, const char*label, item_action *action) {

    menu *sub_menu = malloc(sizeof(menu));

    sub_menu->max_id = -1;
    sub_menu->active_id = -1;
    sub_menu->current_id = 0;

    sub_menu->radius_labels = m->ctrl->radius_labels;
    sub_menu->radius_scales_start = m->ctrl->radius_scales_start;
    sub_menu->radius_scales_end = m->ctrl->radius_scales_end;

    sub_menu->parent = m;
    sub_menu->item = 0;
    sub_menu->segment = 0;
    sub_menu->n_o_lines = 1;
    sub_menu->label = 0;
    sub_menu->object = 0;

    sub_menu->ctrl = m->ctrl;
    sub_menu->n_o_items_on_scale = m->n_o_items_on_scale;
    sub_menu->n_o_segments = m->n_o_segments;

    menu_item *item = menu_item_new(m, label, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;
    item->is_sub_menu = 1;

    return item;

}

void menu_turn(menu *m, int direction) {
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    ctrl->warping = 1;
    unsigned int i = 0;
    unsigned int total_n_o_segments = m->n_o_items_on_scale * (2.0*m->n_o_segments+1);
    for (i = 0; i < 1; i++) {
        m->segment = m->segment + direction;
        if (m->segment > m->n_o_segments) {
            m->segment = -m->n_o_segments;
            m->current_id = m->current_id - 1;
            if (m->current_id < 0) {
                m->current_id = m->max_id;
            }
        } else if (m->segment < -m->n_o_segments) {
            m->segment = m->n_o_segments;
            m->current_id = m->current_id + 1;
            if (m->current_id > m->max_id) {
                m->current_id = 0;
            }
        }
        ctrl->bg_segment = ctrl->bg_segment + direction;
        if (ctrl->bg_segment >= total_n_o_segments) {
            ctrl->bg_segment = 0;
        } else if (ctrl->bg_segment < 0) {
            ctrl->bg_segment = total_n_o_segments - 1;

        }
        menu_ctrl_draw(m->ctrl);
    }
}

void menu_turn_right(menu *m) {
    menu_turn(m,1);
}

void menu_turn_left(menu *m) {
    menu_turn(m,-1);
}

menu_ctrl *menu_get_ctrl(menu *m) {
    return m->ctrl;
}

int menu_get_current_id(menu *menu) {
    return menu->current_id;
}

void menu_set_current_id(menu *menu, int id) {
    if (menu->max_id >= id) {
        menu->current_id = id;
        if (menu_item_is_sub_menu(menu_get_item(menu,id))) {
        }
    }
}

void menu_set_radius(menu *m, int radius_labels, int radius_scales_start, int radius_scales_end) {

    if (radius_labels >= 0)
        m->radius_labels = radius_labels;
    if (radius_scales_start >= 0)
        m->radius_scales_start = radius_scales_start;
    if (radius_scales_end >= 0)
        m->radius_scales_end = radius_scales_end;

    int i = 0;
    for (i = 0; i < m->max_id; i++) {
        if (m->item[i] && m->item[i]->is_sub_menu) {
            menu_set_radius((menu *) m->item[i]->sub_menu, radius_labels, radius_scales_start, radius_scales_end);
        }
    }

}

void menu_update_cnt_rad(menu *m, SDL_Point center, int radius) {
    for (int i = 0; i < m->max_id; i++) {
        if (m->item[i]) {
            if (m->item[i]->is_sub_menu) {
                menu_update_cnt_rad((menu *) m->item[i]->sub_menu, center, radius);
            } else {
                menu_item_update_cnt_rad(m->item[i],center,radius);
            }
        }
    }
}

void menu_rebuild_glyphs(menu *m) {
    for (int i = 0; i < m->max_id; i++) {
        if (m->item[i]) {
            if (m->item[i]->is_sub_menu) {
                menu_rebuild_glyphs((menu *) m->item[i]->sub_menu);
            } else {
                menu_item_rebuild_glyphs(m->item[i]);
            }
        }
    }
}

int open_parent_menu(void *ctrl_ptr) {
    menu_ctrl *ctrl = (menu_ctrl *) ctrl_ptr;
        menu *current = ctrl->current;
    if (current->parent) {
        menu_fade_in(current, current->parent);
        ctrl->current = current->parent;
        if (!current->transient) {
            ctrl->active = ctrl->current;
        }
        if (ctrl->action) {
            ctrl->action(CLOSE,current->parent,NULL);
        }
        return 1;
    }
    return 0;
}

int menu_action(menu_event evt, menu_ctrl *ctrl, menu_item *item) {
    if (item) {
        if(item->is_sub_menu) {

            if (item->action) {
                ((item_action*)item->action)(evt, item->menu, item);
            }

            return menu_open_sub_menu(ctrl, item);
        } else if (item->object_type == OBJECT_TYPE_ACTION && item->action) {
            return ((item_action*)item->action)(evt, item->menu, item);
        }
    }
    if (ctrl->action) {
        return ctrl->action(evt, ctrl->current, item);
    }
    return 1;
}

void menu_ctrl_set_offset(menu_ctrl *ctrl, int x_offset, int y_offset) {
    ctrl->x_offset = x_offset;
    ctrl->y_offset = y_offset;
    SDL_Point center;

    center.x = ctrl->x_offset + ctrl->w / 2.0;
    center.y = ctrl->y_offset + 0.5 * ctrl->offset * ctrl->w;

    ctrl->center = center;

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_update_cnt_rad(ctrl->root[r],ctrl->center,ctrl->radius_labels);
    }

}

int menu_ctrl_get_x_offset(menu_ctrl *ctrl) {
    return ctrl->x_offset;
}

void menu_ctrl_set_x_offset(menu_ctrl *ctrl, int x) {
    menu_ctrl_set_offset(ctrl,x,ctrl->y_offset);
}

int menu_ctrl_get_y_offset(menu_ctrl *ctrl) {
    return ctrl->y_offset;
}

void menu_ctrl_set_y_offset(menu_ctrl *ctrl, int y) {
    ctrl->y_offset = y;
}

void menu_ctrl_set_angle_offset(menu_ctrl *ctrl, double a) {
    ctrl->angle_offset = a;
}

int menu_ctrl_get_label_radius(menu_ctrl *ctrl) {
    return ctrl->root[0]->radius_labels;
}

void menu_ctrl_set_label_radius(menu_ctrl *ctrl, int radius) {

    if (ctrl->root) {
        for (int r = 0; r < ctrl->n_roots; r++) {
            menu_set_radius(ctrl->root[r], radius, ctrl->root[r]->radius_scales_start, ctrl->root[r]->radius_scales_end);
        }
    }

}

int menu_ctrl_get_scales_radius_start(menu_ctrl *ctrl) {
    if (ctrl->root) {
        return ctrl->root[0]->radius_scales_start;
    }
    return 0;
}

void menu_ctrl_set_scales_radius_start(menu_ctrl *ctrl, int radius) {

    if (ctrl->root) {
        for (int r = 0; r < ctrl->n_roots; r++) {
            menu_set_radius(ctrl->root[r], ctrl->root[r]->radius_labels, radius, ctrl->root[r]->radius_scales_end);
        }
    }

}

int menu_ctrl_get_scales_radius_end(menu_ctrl *ctrl) {
    if (ctrl->root) {
        return ctrl->root[0]->radius_scales_end;
    }
    return 0;
}

void menu_ctrl_set_scales_radius_end(menu_ctrl *ctrl, int radius) {

    if (ctrl->root) {
        for (int r = 0; r < ctrl->n_roots; r++) {
            menu_set_radius(ctrl->root[r], ctrl->root[r]->radius_labels, ctrl->root[r]->radius_scales_end, radius);
        }
    }

}

menu *menu_ctrl_get_current(menu_ctrl *ctrl) {
    return ctrl->current;
}

void menu_ctrl_set_current(menu_ctrl *ctrl, menu *menu) {
    ctrl->current = menu;
}

menu *menu_ctrl_get_root(menu_ctrl *ctrl) {
    return ctrl->root[0];
}

int menu_ctrl_draw_scale(menu_ctrl *ctrl, double xc, double yc, double r, double R, double angle, unsigned char alpha, int lines) {
    double a = M_PI * angle / 180.0;
    double cos_a = cos(a);
    double sin_a = sin(a);
    double fx1 = xc + r * cos_a;
    double fy1 = yc - r * sin_a;
    double fx2 = xc + R * cos_a;
    double fy2 = yc - R * sin_a;
    if (lines == 1) {
        SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctrl->renderer, ctrl->scale_color->r, ctrl->scale_color->g, ctrl->scale_color->b, alpha/2);
        SDL_RenderDrawLineF(ctrl->renderer, fx1-1, fy1, fx2-1, fy2);
        SDL_RenderDrawLineF(ctrl->renderer, fx1+1, fy1, fx2+1, fy2);
    }
    SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ctrl->renderer, ctrl->scale_color->r, ctrl->scale_color->g, ctrl->scale_color->b, alpha);
    SDL_RenderDrawLineF(ctrl->renderer, fx1, fy1, fx2, fy2);
    return 0;
}

int menu_ctrl_apply_light(menu_ctrl *ctrl) {
    if (ctrl->light_texture) {
        double w = ctrl->w;
        double h = ctrl->h;
        double xo = ctrl->center.x - 0.5 * ctrl->w;
        double yo = 0;
        const SDL_Rect dst_rect = {xo, yo, w, h};
        SDL_RenderCopy(ctrl->renderer,ctrl->light_texture,NULL,&dst_rect);
    }

    return 0;

}

int menu_ctrl_clear(menu_ctrl *ctrl, double angle) {
    if (ctrl->background_color) {
        SDL_SetRenderDrawColor(ctrl->renderer,ctrl->background_color->r,
                               ctrl->background_color->g,
                               ctrl->background_color->b, 255);
        if (SDL_RenderClear(ctrl->renderer) < 0) {
            log_error(MENU_CTX, "Failed to render background: %s\n", SDL_GetError());
            return 0;
        }
    }

    if (ctrl->bg_image) {
        int w, h;
        SDL_QueryTexture(ctrl->bg_image,NULL,NULL,&w,&h);
        double xo = ctrl->center.x - 0.5 * w;
        double yo = ctrl->center.y - 0.5 * h;
        const SDL_FRect dst_rect = {xo, yo, w, h};
        double xc = 0.5 * dst_rect.w;
        double yc = 0.5 * dst_rect.h;
        const SDL_FPoint center = {xc,yc};

        if (SDL_RenderCopyExF(ctrl->renderer, ctrl->bg_image, NULL, &dst_rect, angle, &center, SDL_FLIP_NONE) == -1) {
            log_error(MENU_CTX, "Failed to render background: %s\n", SDL_GetError());
            return 0;
        }
    }

    //menu_ctrl_apply_light(ctrl);

    return 1;
}

void menu_ctrl_set_radii(menu_ctrl *ctrl, int radius_labels, int radius_scales_start, int radius_scales_end) {

    if (ctrl->root) {
        for (int r = 0; r < ctrl->n_roots; r++) {
            menu_set_radius(ctrl->root[r], radius_labels, radius_scales_start, radius_scales_end);
        }
    }

}

int menu_ctrl_set_bg_color_hsv(menu_ctrl *ctrl, double h, double s, double v) {
    log_config(MENU_CTX, "free(ctrl->background_color -> %p)\n", ctrl->background_color);
    free(ctrl->background_color);
    ctrl->background_color = hsv_to_color(h, s, v);
    return 1;
}

int menu_ctrl_set_default_color_hsv(menu_ctrl *ctrl, double h, double s, double v) {
    log_config(MENU_CTX, "free(ctrl->default_color -> %p)\n", ctrl->default_color);
    free(ctrl->default_color);
    ctrl->default_color = hsv_to_color(h, s, v);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_set_active_color_hsv(menu_ctrl *ctrl, double h, double s, double v) {
    log_config(MENU_CTX, "free(ctrl->activated_color -> %p)\n", ctrl->activated_color);
    free(ctrl->activated_color);
    ctrl->activated_color = hsv_to_color(h, s, v);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_set_selected_color_hsv(menu_ctrl *ctrl, double h, double s, double v) {
    log_config(MENU_CTX, "free(ctrl->selected_color -> %p)\n", ctrl->selected_color);
    free(ctrl->selected_color);
    ctrl->selected_color = hsv_to_color(h, s, v);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_set_bg_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b) {
    log_config(MENU_CTX, "free(ctrl->background_color -> %p)\n", ctrl->background_color);
    free(ctrl->background_color);
    ctrl->background_color = rgb_to_color(r,g,b);
    return 1;
}

int menu_ctrl_set_default_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b) {
    log_config(MENU_CTX, "free(ctrl->default_color -> %p)\n", ctrl->default_color);
    free(ctrl->default_color);
    ctrl->default_color = rgb_to_color(r,g,b);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_set_active_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b) {
    log_config(MENU_CTX, "menu_ctrl_set_active_color_rgb: (%d,%d,%d)\n", ctrl->activated_color->r,ctrl->activated_color->g,ctrl->activated_color->b);
    free(ctrl->activated_color);
    ctrl->activated_color = rgb_to_color(r,g,b);
    ctrl->default_color = ctrl->activated_color;
    ctrl->selected_color = ctrl->activated_color;
    log_config(MENU_CTX, " -> (%d,%d,%d)\n", ctrl->activated_color->r,ctrl->activated_color->g,ctrl->activated_color->b);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_set_selected_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b) {
    log_config(MENU_CTX, "free(ctrl->selected_color -> %p)\n", ctrl->selected_color);
    free(ctrl->selected_color);
    ctrl->selected_color = rgb_to_color(r,g,b);

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    return 1;
}

int menu_ctrl_draw(menu_ctrl *ctrl) {
    return menu_draw(ctrl->current, 1, 1);
}

int menu_ctrl_set_style(menu_ctrl *ctrl, char *background, char *scale, char *indicator,
                        char *def, char *selected, char *activated, char *bgImagePath, int bg_from_time, int draw_scales, int font_bumpmap, int shadow_offset, Uint8 shadow_alpha, char **bg_color_palette, int bg_cp_colors, char **fg_color_palette, int fg_cp_colors) {
    log_config(MENU_CTX, "START: menu_ctrl_set_style (");
    log_config(MENU_CTX, "background -> %s", background);
    log_config(MENU_CTX, ", scale -> %s", scale);
    log_config(MENU_CTX, ", def -> %s", def);
    log_config(MENU_CTX, ", selected -> %s", selected);
    log_config(MENU_CTX, ", activated -> %s", activated);
    log_config(MENU_CTX, ", indicator -> %s", indicator);
    log_config(MENU_CTX, ", bgImagePath -> %s", bgImagePath);
    log_config(MENU_CTX, ")\n");
    if (ctrl->background_color) {
        free(ctrl->background_color);
    }
    ctrl->background_color = html_to_color(background);

    if (ctrl->scale_color) {
        free(ctrl->scale_color);
    }
    ctrl->scale_color = html_to_color(scale);

    if (ctrl->default_color) {
        free(ctrl->default_color);
    }
    ctrl->default_color = html_to_color(def);

    if (ctrl->selected_color) {
        free(ctrl->selected_color);
    }
    ctrl->selected_color = html_to_color(selected);

    if (ctrl->activated_color) {
        free(ctrl->activated_color);
    }
    ctrl->activated_color = html_to_color(activated);

    if (ctrl->indicator_color) {
        free(ctrl->indicator_color);
    }
    ctrl->indicator_color = html_to_color_and_alpha(indicator,&(ctrl->indicator_alpha));

    if (ctrl->indicator_color_light) {
        free(ctrl->indicator_color_light);
    }
    ctrl->indicator_color_light = color_between(ctrl->indicator_color, &white, 0.85);

    if (ctrl->indicator_color_dark) {
        free(ctrl->indicator_color_dark);
    }
    ctrl->indicator_color_dark = color_between(ctrl->indicator_color, &black, 0.85);

    if (bg_from_time == 1) {
        ctrl->bg_color_of_time = 1;
    } else {
        ctrl->bg_color_of_time = 0;
    }

    if (ctrl->bg_image) {
        SDL_DestroyTexture(ctrl->bg_image);
        ctrl->bg_image = NULL;
    }

    if (bgImagePath) {
        ctrl->bg_image = IMG_LoadTexture(ctrl->renderer,bgImagePath);
        if (!ctrl->bg_image) {
            log_error(MENU_CTX, "Could not load background image %s: %s\n", bgImagePath, SDL_GetError());
        }
    } else {
        ctrl->bg_image = NULL;
    }

    if (bg_color_palette) {
        ctrl->bg_color_palette = bg_color_palette;
        ctrl->bg_cp_colors = bg_cp_colors;
    }

    if (fg_color_palette) {
        ctrl->fg_color_palette = fg_color_palette;
        ctrl->fg_cp_colors = fg_cp_colors;
    }

    ctrl->font_bumpmap = font_bumpmap;
    ctrl->shadow_offset = shadow_offset;
    ctrl->shadow_alpha = shadow_alpha;

    log_info(MENU_CTX, "Colors:\n");
    html_print_color("Background", ctrl->background_color);
    html_print_color("Default", ctrl->default_color);
    html_print_color("Scale", ctrl->scale_color);
    html_print_color("Indicator", ctrl->indicator_color);
    html_print_color("Selected", ctrl->selected_color);
    html_print_color("Activated", ctrl->activated_color);

    int draw_res = menu_ctrl_draw(ctrl);
    log_config(MENU_CTX, "END: menu_ctrl_set_style\n");
    return draw_res;

}

int menu_ctrl_apply_theme(menu_ctrl *ctrl, theme *theme) {
    return menu_ctrl_set_style(ctrl, theme->background_color, theme->scale_color,
                               theme->indicator_color, theme->default_color, theme->selected_color,
                               theme->activated_color, theme->bg_image_path, theme->bg_color_of_time, 1, theme->font_bumpmap, theme->shadow_offset, theme->shadow_alpha, theme->bg_color_palette, theme->bg_cp_colors, theme->fg_color_palette, theme->fg_cp_colors);
}

void menu_ctrl_set_light(menu_ctrl *ctrl, double light_x, double light_y, double radius, double alpha) {
    if (ctrl->light_texture) {
        SDL_DestroyTexture(ctrl->light_texture);
        free(ctrl->light_texture);
    }
    ctrl->light_x = light_x;
    ctrl->light_y = light_y;

    ctrl->light_texture = new_light_texture(ctrl->renderer, ctrl->w, ctrl->h, light_x, light_y, radius, alpha);

}

void menu_ctrl_set_light_img(menu_ctrl *ctrl, char *path) {
    if (ctrl->light_texture) {
        SDL_DestroyTexture(ctrl->light_texture);
    }

    ctrl->light_texture = load_light_texture(ctrl->renderer,path);

}

menu_ctrl *menu_ctrl_new(int w, int x_offset, int y_offset, int radius_labels, int draw_scales, int radius_scales_start, int radius_scales_end, double angle_offset, const char *font, int font_size, int font_size2,
                         item_action *action, menu_callback *call_back) {

    log_config(MENU_CTX, "Initializing menu of width %d\n", w);
    menu_ctrl *ctrl = calloc(1,sizeof(menu_ctrl));

    ctrl->warping = 0;

    ctrl->radius_labels = radius_labels;
    ctrl->radius_scales_start = radius_scales_start;
    ctrl->radius_scales_end = radius_scales_end;
    ctrl->draw_scales = draw_scales;
    ctrl->no_of_scales = 32;
    ctrl->n_o_items_on_scale = 4;
    ctrl->n_o_segments = 3;

    ctrl->w = w;
    ctrl->h = to_int(0.65 * w);

    ctrl->root = malloc(sizeof(menu *));
    ctrl->n_roots = 1;

    menu *root = malloc(sizeof(menu));

    root->max_id = -1;
    root->item = 0x0;
    root->active_id = -1;
    root->current_id = 0;
    root->segment = 0;
    root->parent = 0;
    root->label = 0;
    root->transient = 0;
    root->draw_only_active = 0;
    root->object = 0;
    root->n_o_lines = 1;
    root->n_o_items_on_scale = ctrl->n_o_items_on_scale;
    root->n_o_segments = ctrl->n_o_segments;

    ctrl->root[0] = root;

    ctrl->offset = 1.2;

    menu_ctrl_set_offset(ctrl,x_offset,y_offset);

    ctrl->font_size = font_size;
    if (font_size2 > 0) {
        ctrl->font_size2 = font_size2;
    } else {
        ctrl->font_size2 = font_size - 8;
    }

    ctrl->light_x = 0.0;
    ctrl->light_y = 0.0;

    ctrl->angle_offset = angle_offset;

    ctrl->current = ctrl->root[0];
    ctrl->active = ctrl->root[0];
    ctrl->root[0]->ctrl = ctrl;

    menu_ctrl_set_radii(ctrl,radius_labels,radius_scales_start,radius_scales_end);

    ctrl->call_back = call_back;
    ctrl->action = action;
    ctrl->bg_image = 0;

    ctrl->bg_color_palette = NULL;
    ctrl->bg_cp_colors = 0;

    log_info(MENU_CTX, "Initializing SDL2");
    if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_EVENTS) == -1) {
        log_error(MENU_CTX, "...SDL init failed: %s\n", SDL_GetError());
        return 0;
    } else {
        log_info(MENU_CTX, "...SDL init succeeded\n");
    }

    int v = 0;
    log_info(MENU_CTX, "Available video drivers:\n");
    for (v = 0; v < SDL_GetNumVideoDrivers(); v++) {
        log_info(MENU_CTX, "%s\n", SDL_GetVideoDriver(v));
    }
    log_info(MENU_CTX, "Available video renderers:\n");
    int r = 0;
    for (r = 0; r < SDL_GetNumRenderDrivers(); r++) {
        SDL_RendererInfo rendererInfo;
        SDL_GetRenderDriverInfo(r, &rendererInfo);
        log_info(MENU_CTX, "%d ", r );
        log_info(MENU_CTX, "Renderer: %s software=%d accelerated=%d, presentvsync=%d targettexture=%d\n",
                  rendererInfo.name,
                  (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0,
                  (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0,
                  (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0,
                  (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0 );
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");

    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF);

    log_info(MENU_CTX, "Initializing TTF");
    if (TTF_Init() == -1) {
        log_error(MENU_CTX, "...TTF init failed: %s\n", SDL_GetError());
        return 0;
    } else {
        log_info(MENU_CTX, "...TTF init succeeded\n");
    }

    if (font) {
        log_config(MENU_CTX, "Trying to open font %s\n", font);
        ctrl->font = TTF_OpenFont(font, font_size);
        ctrl->font2 = TTF_OpenFont(font, ctrl->font_size2);
    }

    if (!ctrl->font) {
        log_error(MENU_CTX, "Failed to load font %s: %s. Trying %s\n", font, SDL_GetError(), FONT_DEFAULT);
        ctrl->font = TTF_OpenFont(FONT_DEFAULT, font_size);
        ctrl->font2 = TTF_OpenFont(FONT_DEFAULT, ctrl->font_size2);
        if (!ctrl->font) {
            log_error(MENU_CTX, "Failed to load font %s: %s. Trying /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf.\n", FONT_DEFAULT, SDL_GetError());
            ctrl->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", font_size);
            ctrl->font2 = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", ctrl->font_size2);
            if (!ctrl->font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
    } else {
        log_config(MENU_CTX, "Font %s successfully opened\n", font);
    }

    log_info(MENU_CTX, "Creating window...\n");
    ctrl->display = SDL_CreateWindow("VE301 (SDL2)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ctrl->w, ctrl->h, 0);
    if (ctrl->display) {
        ctrl->renderer = SDL_CreateRenderer(ctrl->display, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_RendererInfo rendererInfo;
        SDL_GetRendererInfo(ctrl->renderer, &rendererInfo);
        log_info(MENU_CTX, "SDL chose the following renderer:\n");
        log_info(MENU_CTX, "Renderer: %s software=%d accelerated=%d, presentvsync=%d targettexture=%d\n",
                  rendererInfo.name,
                  (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0,
                  (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0,
                  (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0,
                  (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0 );

    }

    if (!ctrl->display) {
        log_error(MENU_CTX, "Failed to create window: %s\n", SDL_GetError());
        return 0;
    }

#ifdef RASPBERRY
    menu_ctrl_set_style (ctrl, "#08081e", "#c8c8c8", "#ff0000", "#525239", "#c8c864", "#c8c864", 0, 0, 1, 0, 0, 0, NULL, 0, NULL, 0);
#else
    menu_ctrl_set_style(ctrl, "#08081e", "#c8c8c8", "#ff0000", "#525239", "#c8c864",
                        "#c8c864",
                        0, 0, 1, 0, 0, 0, NULL, 0, NULL, 0);
#endif

    return ctrl;
}

int menu_ctrl_process_events(menu_ctrl *ctrl) {
    int redraw = 0;

#ifdef RASPBERRY
    int he = next_event ();
    while (he) {
        ctrl->warping = 1;
        log_config (MENU_CTX, "Event: %d\n", he);
        if (he == BUTTON_A_TURNED_LEFT) {
            menu_turn_left (ctrl->current);
            menu_action (TURN_LEFT, ctrl, 0);
            redraw = 1;
        } else if (he == BUTTON_A_TURNED_RIGHT) {
            menu_turn_right (ctrl->current);
            menu_action (TURN_RIGHT, ctrl, 0);
            redraw = 1;
        } else if (he == BUTTON_A_PRESSED) {
            if (ctrl->current->current_id >= 0 && ctrl->current->item) {
                menu_item *item = ctrl->current->item[ctrl->current->current_id];
                menu_action (ACTIVATE, ctrl, item);
                redraw = 1;
            } else {
                log_trace (MENU_CTX, "No action.\n");
            }
        } else if (he == BUTTON_A_HOLD) {
            open_parent_menu (ctrl);
        } else if (he == BUTTON_B_TURNED_LEFT) {
            menu_item *item = NULL;
            if (ctrl->current->current_id >= 0) {
                item = ctrl->current->item[ctrl->current->current_id];
            }
            menu_action(TURN_LEFT_1, ctrl, item);
        } else if (he == BUTTON_B_TURNED_RIGHT) {
            menu_item *item = NULL;
            if (ctrl->current->current_id >= 0) {
                item = ctrl->current->item[ctrl->current->current_id];
            }
            menu_action(TURN_RIGHT_1, ctrl, item);
        } else if (he == BUTTON_B_PRESSED) {
            menu_action (ACTIVATE_1, ctrl, 0);
            //open_parent_menu(ctrl);
        }
        he = next_event ();
    }
    return redraw;
#else
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return -1;
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            SDL_MouseButtonEvent *b = (SDL_MouseButtonEvent *) &e;
            if (b->state == SDL_RELEASED) {
                if (b->button == 1) {
                    if (ctrl->current->current_id >= 0) {
                        menu_item *item =
                                ctrl->current->item[ctrl->current->current_id];
                        menu_action(ACTIVATE, ctrl, item);
                        redraw = 1;
                    } else {
                        log_trace(MENU_CTX, "No action.\n");
                    }
                } else if (b->button == 3) {
                    if (ctrl->current->parent) {
                        open_parent_menu(ctrl);
                        redraw = 1;
                    }
                } else if (b->button == 4) {
                    menu_turn_left(ctrl->current);
                    menu_action(TURN_LEFT, ctrl, 0);
                    redraw = 1;
                } else if (b->button == 5) {
                    menu_turn_right(ctrl->current);
                    menu_action(TURN_RIGHT, ctrl, 0);
                    redraw = 1;
                }
            }
        } else if (e.type == SDL_MOUSEWHEEL) {
            SDL_MouseWheelEvent *w = (SDL_MouseWheelEvent *) &e;
            if (w->y > 0) {
                menu_turn_left(ctrl->current);
                menu_action(TURN_LEFT, ctrl, 0);
                redraw = 1;
            } else if (w->y < 0) {
                menu_turn_right(ctrl->current);
                menu_action(TURN_RIGHT, ctrl, 0);
                redraw = 1;
            }
        } else if (e.type == SDL_KEYUP) {
            SDL_KeyboardEvent *k = (SDL_KeyboardEvent *) &e;
            if (k->state == SDL_RELEASED) {
                if (k->keysym.sym == SDLK_UP) {
                    menu_item *item = NULL;
                    if (ctrl->current->current_id >= 0) {
                        item = ctrl->current->item[ctrl->current->current_id];
                    }
                    menu_action(TURN_RIGHT_1, ctrl, item);
                } else if (k->keysym.sym == SDLK_DOWN) {
                    menu_item *item = NULL;
                    if (ctrl->current->current_id >= 0) {
                        item = ctrl->current->item[ctrl->current->current_id];
                    }
                    menu_action(TURN_LEFT_1, ctrl, item);
                } else if (k->keysym.sym == SDLK_q) {
                    return -1;
                }
            }
        }
    }
    return redraw;
#endif
}

void menu_free(menu *m);

void menu_item_free(menu_item *item) {
    if (item) {
        if (item->label_active) {
            text_obj_free(item->label_active);
        }
        if (item->label_current) {
            text_obj_free(item->label_current);
        }
        if (item->label_default) {
            text_obj_free(item->label_default);
        }
        if (item->object) {
            if (item->is_sub_menu) {
                menu_free((menu *)item->object);
            } else {
                free((void *) item->object);
            }
        }
        free(item);
    }
}

void menu_free(menu *m) {
    if (m) {
        for (int i = 0; i < m->max_id; i++) {
            menu_item_free(m->item[i]);
        }
        if (m->label) {
            free(m->label);
        }
        if (m->object) {
            free((void *) m->object);
        }
        free(m);
    }
}

void menu_ctrl_free(menu_ctrl *ctrl) {
    if (ctrl) {
        if (ctrl->activated_color) {
            free(ctrl->activated_color);
        }
        if (ctrl->background_color) {
            free(ctrl->background_color);
        }
        if (ctrl->default_color) {
            free(ctrl->default_color);
        }
        if (ctrl->indicator_color) {
            free(ctrl->indicator_color);
        }
        if (ctrl->indicator_color_dark) {
            free(ctrl->indicator_color_dark);
        }
        if (ctrl->indicator_color_light) {
            free(ctrl->indicator_color_light);
        }
        if (ctrl->scale_color) {
            free(ctrl->scale_color);
        }
        if (ctrl->selected_color) {
            free(ctrl->selected_color);
        }
        if (ctrl->root) {
            for (int r = 0; r < ctrl->n_roots; r++) {
                menu_free(ctrl->root[r]);
            }
            ctrl->root = NULL;
        }
        if (ctrl->object) {
            free(ctrl->object);
        }
        if (ctrl->renderer) {
            SDL_DestroyRenderer(ctrl->renderer);
        }
        if (ctrl->display) {
            SDL_DestroyWindow(ctrl->display);
        }
        free(ctrl);
    }
}

void menu_ctrl_quit(menu_ctrl *ctrl) {
    log_info(MENU_CTX, "Cleaning up menu ctrl");
    menu_ctrl_free(ctrl);
    log_info(MENU_CTX, "Closing TTF\n");
    TTF_Quit();
    log_info(MENU_CTX, "Closing IMG\n");
    IMG_Quit();
    log_info(MENU_CTX, "Closing SDL\n");
    SDL_Quit();
}

void menu_ctrl_loop(menu_ctrl *ctrl) {

    log_config(MENU_CTX, "START: menu_ctrl_loop\n");
#ifdef RASPBERRY
    setup_encoder ();
#endif

    menu_ctrl_draw(ctrl);

    while (1) {
        int res = menu_ctrl_process_events(ctrl);
        log_trace(MENU_CTX, "events result: %d\n", res);
        if (res == -1) {
            menu_ctrl_quit(ctrl);
            return;
        } else if (res == 0) {
            if (ctrl->call_back) {
                ctrl->call_back(ctrl);
            }
            SDL_Delay(20);
        } else {
            menu_ctrl_draw(ctrl);
        }
    }
    log_config(MENU_CTX, "END: menu_ctrl_loop\n");
}
