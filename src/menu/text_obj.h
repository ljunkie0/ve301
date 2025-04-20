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

text_obj *text_obj_new(SDL_Renderer *renderer, char *txt, TTF_Font *font, TTF_Font *font_2nd_line, SDL_Color fg, SDL_Point center, int radius, int line, int n_lines, int light_x, int light_y);
void text_obj_free(text_obj *obj);
void text_obj_draw(SDL_Renderer *renderer, SDL_Texture *target, text_obj *label, int radius, int center_x, int center_y, double angle, double light_x, double light_y, int font_bumpmap, int shadow_offset, int shadow_alpha);
void text_obj_update_cnt_rad(text_obj *obj, SDL_Point center, int radius, int line, int n_lines);

#endif // TEXT_OBJ_H
