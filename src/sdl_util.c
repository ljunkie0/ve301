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

#include <math.h>
#include"sdl_util.h"
#include"base.h"

float Q_rsqrt( float number ) {
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

SDL_Color *html_to_color_and_alpha(char *c, unsigned char *alpha) {
    SDL_Color *color = 0;
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

char *rgb_to_html(int r, int g, int b) {
    char *html = malloc(10*sizeof(char));
    sprintf(html,"#%02x%02x%02x",r,g,b);
    return html;
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

void color_temp_to_rgb(double temp, Uint8 *r, Uint8 *g, Uint8 *b, double value) {
    temp = temp / 100.0;

    double rd, gd, bd;

    if (temp <= 66.0) {
        rd = 255.0;
    } else {
        rd = temp - 60.0;
        rd = 329.698727446 * pow(rd,-0.1332047592);
        if (rd < 0.0) {
            rd = 0.0;
        } else if (rd > 255.0) {
            rd = 255.0;
        }
    }

    if (temp <= 66.0) {
        gd = temp;
        gd = 99.4708025861 * log(gd) - 161.1195681661;
        if (gd < 0.0) {
            gd = 0.0;
        } else if (gd > 255.0) {
            gd = 255.0;
        }
    } else {
        gd = temp - 60.0;
        gd = 288.1221695283 * pow(gd,-0.0755148492);
        if (gd < 0.0) {
            gd = 0.0;
        } else if (gd > 255.0) {
            gd = 255.0;
        }
    }

    if (temp >= 66.0) {
        bd = 255.0;
    } else {
        if (temp <= 19.0) {
            bd = 0.0;
        } else {
            bd = temp - 10.0;
            bd = 138.5177312231 * log(bd) - 305.0447927307;
            if (bd < 0.0) {
                bd = 0.0;
            } else if (bd > 255.0) {
                bd = 255.0;
            }
        }
    }

    *r = to_Uint8(rd * value);
    *g = to_Uint8(gd * value);
    *b = to_Uint8(bd * value);

    log_info(SDL_CTX, "color_temp_to_rgb(%f) -> %d/%d/%d\n", temp, *r, *g, *b);

}

void rgb_to_hsv(Uint8 r, Uint8 g, Uint8 b, double *h, double *s, double *v) {

        double      min, max, delta;

        double rf = 100.0 * ((double)r) / 255.0;
        double gf = 100.0 * ((double)g) / 255.0;
        double bf = 100.0 * ((double)b) / 255.0;

        min = rf < gf ? rf : gf;
        min = min  < bf ? min  : bf;

        max = rf > gf ? rf : gf;
        max = max  > bf ? max  : bf;

        *v = max;                                // v
        delta = max - min;
        if (delta < 0.00001)
        {
            *s = 0;
            *h = 0; // undefined, maybe nan?
            return;
        }
        if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
            *s = (delta / max);                  // s
        } else {
            // if max is 0, then r = g = b = 0
            // s = 0, h is undefined
            *s = 0.0;
            *h = NAN;                            // its now undefined
            return;
        }
        if( rf >= max )                           // > is bogus, just keeps compilor happy
            *h = ( gf - bf ) / delta;        // between yellow & magenta
        else
            if( gf >= max )
                *h = 2.0 + ( bf - rf ) / delta;  // between cyan & yellow
            else
                *h = 4.0 + ( rf - gf ) / delta;  // between magenta & cyan

        *h *= 60.0;                              // degrees

        if( *h < 0.0 )
            *h += 360.0;

        return;
}

void hsv_to_rgb(double h, double s, double v, Uint8 *r, Uint8 *g, Uint8 *b) {

    double hh, p, q, t, ff;
    long i;
    double rf, gf, bf;

    v = v / 100.0;

    if (s <= 0.0) { // < is bogus, just shuts up warnings
        Uint8 c = to_Uint8(v * 255.0);
        *r = c;
        *g = c;
        *b = c;
    }

    hh = h;
    if (hh >= 360.0)
        hh = 0.0;

    hh /= 60.0;
    i = (long) hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch (i) {
    case 0:
        rf = v;
        gf = t;
        bf = p;
        break;
    case 1:
        rf = q;
        gf = v;
        bf = p;
        break;
    case 2:
        rf = p;
        gf = v;
        bf = t;
        break;

    case 3:
        rf = p;
        gf = q;
        bf = v;
        break;
    case 4:
        rf = t;
        gf = p;
        bf = v;
        break;
    case 5:
    default:
        rf = v;
        gf = p;
        bf = q;
        break;
    }

    *r = to_Uint8(255 * rf);
    *g = to_Uint8(255 * gf);
    *b = to_Uint8(255 * bf);

    log_debug(SDL_CTX, "hsv_to_rgb: (%f, %f, %f) -> (%d, %d, %d)\n", h, s, v, *r, *g, *b);

}

SDL_Color *hsv_to_color(double h, double s, double v) {

    SDL_Color *color = malloc(sizeof(SDL_Color));

    hsv_to_rgb(h,s,v,&(color->r), &(color->g), &(color->b));

    return color;
}

void apply_background_alpha(SDL_Color *fg_color, SDL_Color *bg_color, int alpha) {
    fg_color->r = to_Uint8(alpha * fg_color->r / 255 + (255 - alpha) * bg_color->r / 255);
    fg_color->g = to_Uint8(alpha * fg_color->g / 255 + (255 - alpha) * bg_color->g / 255);
    fg_color->b = to_Uint8(alpha * fg_color->b / 255 + (255 - alpha) * bg_color->b / 255);
}

SDL_Texture *create_light_texture(SDL_Renderer *renderer, int w, int h, int light_x, int light_y, int radius, int alpha) {
    SDL_Texture *light_texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,w,h);

    Uint32 pixels[w*h];
    for (int y = 0; y < h; y++) {
        int o = w * y;
        for (int x = 0; x < w; x++) {

            double d = SDL_sqrt((x - light_x)*(x - light_x) + (y - light_y)*(y - light_y));
            double l = 255 - 255 * d / radius;
            if (l < 0) l = 0;
            if (l > 255) l = 255;

            pixels[x+o] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888),l,l,l,alpha);

        }
    }

    SDL_UpdateTexture(light_texture,NULL,pixels,4*w);

    return light_texture;
}
