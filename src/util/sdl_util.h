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

#include<SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define to_int(d) (int) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Uint8(d) (Uint8) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Uint16(d) (Uint16) (d >= 0.0 ? (d + 0.5) : (d - 0.5))
#define to_Sint16(d) (Sint16) (d >= 0.0 ? (d + 0.5) : (d - 0.5))

#define DEFAULT_SDL_PIXELFORMAT SDL_PIXELFORMAT_RGBA32

float Q_rsqrt( float number );
TTF_Font *my_OpenTTF_Font(const char *path, const int size);
SDL_Color *html_to_color_and_alpha(char *c, unsigned char *alpha);
SDL_Color *html_to_color(char *c);
SDL_Color *rgb_to_color(int r, int g, int b);
void color_between_rgb(unsigned char rf, unsigned char gf, unsigned char bf, unsigned char rt, unsigned char gt, unsigned char bt, double t, unsigned char *r, unsigned char *g, unsigned char *b);
SDL_Color *color_between(SDL_Color *from, SDL_Color *to, double t);
SDL_Color *clone_color(SDL_Color *color);
void html_print_color(char *name, SDL_Color *c);
SDL_Texture *new_light_texture(SDL_Renderer *renderer, int w, int h, int light_x, int light_y, int radius, int alpha);
Uint8 get_alpha(Uint32 pixel, SDL_PixelFormat *format);
int init_SDL();
