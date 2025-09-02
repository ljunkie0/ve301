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
    double radius;
    double current_angle;
    double shadow_dx;
    double shadow_dy;
} glyph_obj;

glyph_obj *glyph_obj_new(SDL_Renderer *renderer, uint16_t c, TTF_Font *font, SDL_Color fg, SDL_Point center, int radius);
glyph_obj *glyph_obj_new_surface(SDL_Renderer *renderer, SDL_Surface *surface, TTF_Font *font, SDL_Color fg, SDL_Point center, int radius);
void glyph_obj_free(glyph_obj *obj);
void glyph_obj_update_cnt_rad(glyph_obj *glyph_o, SDL_Point center, int radius);
void glyph_obj_update_bumpmap_texture(SDL_Renderer *renderer, glyph_obj *glyph_o, double center_x, double center_y, int angle, double l_x, double l_y);

#endif // GLYPH_OBJ_H
