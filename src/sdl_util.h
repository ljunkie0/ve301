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

#include<SDL2/SDL.h>

#define to_int(d) (int) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Uint8(d) (Uint8) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Uint16(d) (Uint16) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Sint16(d) (Sint16) (d >= 0.0 ? (d + 0.5) : (d - 0.5))

float Q_rsqrt( float number );
SDL_Color *html_to_color_and_alpha(char *c, unsigned char *alpha);
SDL_Color *html_to_color(char *c);
SDL_Color *rgb_to_color(int r, int g, int b);
char *rgb_to_html(int r, int g, int b);
void color_between_rgb(unsigned char rf, unsigned char gf, unsigned char bf, unsigned char rt, unsigned char gt, unsigned char bt, double t, unsigned char *r, unsigned char *g, unsigned char *b);
SDL_Color *color_between(SDL_Color *from, SDL_Color *to, double t);
SDL_Color *hsv_to_color(double h, double s, double v);
void color_temp_to_rgb(double temp, Uint8 *r, Uint8 *g, Uint8 *b, double value);
void hsv_to_rgb(double h, double s, double v, Uint8 *r, Uint8 *g, Uint8 *b);
void rgb_to_hsv(Uint8 r, Uint8 g, Uint8 b, double *h, double *s, double *v);
void html_print_color(char *name, SDL_Color *c);
void apply_background_alpha(SDL_Color *fg_color, SDL_Color *bg_color, int alpha);
SDL_Texture *create_light_texture(SDL_Renderer *renderer, int w, int h, int x, int y, int radius, int alpha);
