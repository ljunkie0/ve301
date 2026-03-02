/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
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

#include "sdl_util.h"
#include "base/log_contexts.h"
#include "base/logging.h"
#include <math.h>

inline float Q_rsqrt( float number ) {

    return 1/SDL_sqrt(number);

}

Uint8 get_alpha(Uint32 pixel, SDL_PixelFormat *format) {
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
    return a;
}

TTF_Font *my_OpenTTF_Font(const char *path, const int size) {
    if (!path) {
        return NULL;
    }
    if (size == 0) {
        return NULL;
    }
    log_config(MENU_CTX, "Opening font %s with size %d\n", path, size);
    return TTF_OpenFont(path,size);
}

SDL_Color *html_to_color_and_alpha(char *c, unsigned char *alpha) {
    SDL_Color *color = NULL;
    if (c) {
        color = malloc(sizeof(SDL_Color));
        unsigned int r;
        unsigned int g;
        unsigned int b;
        unsigned int a = 255;
        int items_read = sscanf(c, "#%02x%02x%02x%02x", &r, &g, &b, &a);
        color->r = (unsigned char) r;
        color->g = (unsigned char) g;
        color->b = (unsigned char) b;
        if (items_read >= 4) {
            *alpha = (unsigned char) a;
        } else {
            *alpha = 255;
        }
          color->a = *alpha;
    }
    return color;
                                                         }

SDL_Color *html_to_color(char *c) {
    unsigned char alpha;
    return html_to_color_and_alpha(c,&alpha);
}

void html_print_color(char *name, SDL_Color *c) {
    log_info(SDL_CTX, "%s: #%02x%02x%02x\n", name, c->r, c->g, c->b);
}

SDL_Color *rgb_to_color(int r, int g, int b) {
    SDL_Color *color = malloc(sizeof(SDL_Color));
    color->r = (unsigned char) r;
    color->g = (unsigned char) g;
    color->b = (unsigned char) b;
    return color;
}

void color_between_rgb(unsigned char rf, unsigned char gf, unsigned char bf, unsigned char rt, unsigned char gt, unsigned char bt, double t, unsigned char *r, unsigned char *g, unsigned char *b) {
    *r = (unsigned char) (t * ((double) rt) + (1-t) * ((double) rf));
    *g = (unsigned char) (t * ((double) gt) + (1-t) * ((double) gf));
    *b = (unsigned char) (t * ((double) bt) + (1-t) * ((double) bf));
}

SDL_Color *color_between(SDL_Color *from, SDL_Color *to, double t) {
    SDL_Color *color = malloc(sizeof(SDL_Color));
    color_between_rgb(from->r, from->g, from->b, to->r, to->g, to->b, t, &(color->r), &(color->g), &(color->b));
    return color;
}

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

SDL_Color *clone_color(SDL_Color *color) {
    SDL_Color *new_color = malloc(sizeof(SDL_Color));
    new_color->a = color->a;
    new_color->r = color->r;
    new_color->g = color->g;
    new_color->b = color->b;
    return new_color;
}

int init_SDL() {

    log_info(SDL_CTX, "Initializing SDL2");
    if (SDL_Init(/*SDL_INIT_TIMER|*/SDL_INIT_VIDEO|SDL_INIT_EVENTS) == -1) {
        log_error(SDL_CTX, "...SDL init failed: %s\n", SDL_GetError());
        return 0;
    } else {
        log_info(SDL_CTX, "...SDL init succeeded\n");
    }

    int v = 0;
    log_info(SDL_CTX, "Available video drivers:\n");
    for (v = 0; v < SDL_GetNumVideoDrivers(); v++) {
        log_info(SDL_CTX, "%s\n", SDL_GetVideoDriver(v));
    }

    log_info(SDL_CTX, "SDL chose the driver: %s\n", SDL_GetCurrentVideoDriver());

    log_info(SDL_CTX, "Available video renderers:\n");
    int r = 0;
    for (r = 0; r < SDL_GetNumRenderDrivers(); r++) {
        SDL_RendererInfo rendererInfo;
        SDL_GetRenderDriverInfo(r, &rendererInfo);
        log_info(SDL_CTX, "%d ", r );
        log_info(SDL_CTX, "Renderer: %s software=%d accelerated=%d, presentvsync=%d targettexture=%d\n",
                 rendererInfo.name,
                 (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0,
                 (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0,
                 (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0,
                 (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0 );
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");

    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF);

    log_info(SDL_CTX, "Initializing TTF");
    if (TTF_Init() == -1) {
        log_error(SDL_CTX, "...TTF init failed: %s\n", SDL_GetError());
        return 0;
    } else {
        log_info(SDL_CTX, "...TTF init succeeded\n");
    }

    return 1;

}
