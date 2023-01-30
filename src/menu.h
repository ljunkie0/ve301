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

#ifndef MENU_H_
#define MENU_H_

#include<SDL2/SDL.h>
#include<SDL2/SDL_ttf.h>

#include"audio.h"

#define UNKNOWN_OBJECT_TYPE -1
#define OBJECT_TYPE_ACTION -2

typedef enum {
    TURN_LEFT,
    TURN_RIGHT,
    ACTIVATE,
    TURN_LEFT_1,
    TURN_RIGHT_1,
    ACTIVATE_1,
    DISPOSE,
    CLOSE
} menu_event;

typedef struct menu_item menu_item;
typedef struct menu menu;
typedef struct menu_ctrl menu_ctrl;
typedef int item_action(menu_event, menu *, menu_item *);
typedef int menu_callback(menu_ctrl *ctrl);
typedef struct text_obj text_obj;

typedef struct menu_item {
    int id;
    Uint16 *unicode_label;
    Uint16 *unicode_label2;
    char *utf8_label;
    char *utf8_label2;
    char *label;
    int num_label_chars;
    int num_label_chars2;
    int w;
    int h;
    int is_sub_menu;
    void *sub_menu;
    TTF_Font *font;
    TTF_Font *font2;
    menu *menu;
    void **glyph_objs;
    void **glyph_objs2;
    int line; // The line (0 = default, >0 = above, <0 = below)

    text_obj *label_default;
    text_obj *label_active;
    text_obj *label_current;

    float *kernings;
    float *kernings2;
    item_action *action;
    int w2;
    int h2;
    /**
     * User data
    **/
    int object_type;
    const void *object;
} menu_item;

typedef struct menu {
    int max_id;
    char *label;
    int current_id;
    int active_id;
    int radius_labels;
    int radius_scales_start;
    int radius_scales_end;
    int pos;
    int n_o_lines;
    int n_o_segments;
    int n_o_items_on_scale;
    double segment;
    menu_item **item;
    menu *parent;
    menu_ctrl *ctrl;
    uint8_t transient;
    uint8_t draw_only_active;
    /**
     * User data
    **/
    const void *object;
} menu;

typedef struct theme {
    char *background_color;
    int bg_color_of_time; /* Change background color over day */
    char *scale_color;
    char *indicator_color;
    char *default_color;
    char *selected_color;
    char *activated_color;
    char *bg_image_path;
    char **bg_color_palette;
    int bg_cp_colors;
    char **fg_color_palette;
    int fg_cp_colors;
    int font_bumpmap;
    int shadow_offset;
    Uint8 shadow_alpha;
} theme;

struct menu_ctrl {
    int w;
    int h;
    int x_offset;
    int y_offset;
    SDL_Point center;
    double light_x;
    double light_y;
    double angle_offset;
    menu **root;
    int n_roots;
    /**
     * @brief current
     * The current menu to be drawn
     */
    menu *current;
    /**
     * @brief active
     * The current active menu. If a menu is not marked as transient, it will be the
     * set the active one during warping and showing of its menu items. Thus the active
     * one can serve as a return point for transient menus
     */
    menu *active;
    double offset;
    int no_of_scales;
    int n_o_segments;
    int n_o_items_on_scale;
    int draw_scales;
    int radius_labels;
    int radius_scales_start;
    int radius_scales_end;
    TTF_Font *font;
    int font_size;
    TTF_Font *font2;
    int font_size2;
    int bg_color_of_time; /* Change background color over day */
    SDL_Color *scale_color; /* The color of the scales */
    SDL_Color *default_color; /* The default foregound color */
    SDL_Color *selected_color; /* The foreground color of the selected item */
    SDL_Color *activated_color; /* The foreground color of the active item (currently unsupported) */
    SDL_Color *background_color; /* The background color */
    SDL_Color *indicator_color; /* The color of the vertical indicator line */
    SDL_Color *indicator_color_light; /* The color of the vertical indicator line */
    SDL_Color *indicator_color_dark; /* The color of the vertical indicator line */
    int font_bumpmap; /* Apply bumpmap effect to font */
    int shadow_offset; /* The offset of the drop shadow (0 -> no shadow) */
    Uint8 shadow_alpha; /* The alpha of the drop shadow */
    Uint8 indicator_alpha;
    SDL_Window *display;
    SDL_Renderer *renderer;
    SDL_Texture *bg_image;
    char **bg_color_palette;
    int bg_cp_colors;
    char **fg_color_palette;
    int fg_cp_colors;
    double bg_segment;
    theme *theme;
    SDL_Texture *light_texture;
    menu_callback *call_back;
    item_action *action;
    int warping;
    /**
     * User data
    **/
    void *object;
};

menu_item *menu_item_new(menu *m, const char *label, void *object, int object_type, const char *font, int font_size, item_action *action, char *font_2nd_line, int font_size_2nd_line);
void menu_item_activate(menu_item *item);
void menu_item_warp_to(menu_item *item);
void menu_item_show(menu_item *item);
menu_item *menu_item_update_label(menu_item *item, const char *label);
menu_item *menu_item_next(menu *m, menu_item *item);

menu *menu_new(menu_ctrl *ctrl);
menu *menu_new_root(menu_ctrl *ctrl);
menu_item *menu_new_sub_menu(menu *menu, const char*label, item_action *action);
menu_item *menu_add_sub_menu(menu *m, const char*label, menu *sub_menu, item_action *action);
int menu_open_sub_menu(menu_ctrl *ctrl, menu_item *item);
int menu_open(menu *m);
int menu_clear(menu *m);

void menu_ctrl_quit(menu_ctrl *ctrl);
menu_ctrl *menu_ctrl_new(int w, int x_offset, int y_offset, int radius_labels, int draw_scales, int radius_scales_start, int radius_scales_end, double angle_offset, const char *font, int font_size, int font_size2,
        item_action *action, menu_callback *call_back);
void menu_ctrl_loop(menu_ctrl *ctrl);
void menu_ctrl_set_radii(menu_ctrl *ctrl, int radius_labels, int radius_scales_start, int radius_scales_end);
int menu_ctrl_apply_theme(menu_ctrl *ctrl, theme *theme);
int menu_ctrl_set_bg_color_hsv(menu_ctrl *ctrl, double h, double s, double v);
int menu_ctrl_set_default_color_hsv(menu_ctrl *ctrl, double h, double s, double v);
int menu_ctrl_set_active_color_hsv(menu_ctrl *ctrl, double h, double s, double v);
int menu_ctrl_set_selected_color_hsv(menu_ctrl *ctrl, double h, double s, double v);
int menu_ctrl_set_bg_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b);
int menu_ctrl_set_default_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b);
int menu_ctrl_set_active_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b);
int menu_ctrl_set_selected_color_rgb(menu_ctrl *ctrl, Uint8 r, Uint8 g, Uint8 b);
void menu_ctrl_set_light(menu_ctrl *ctrl, double light_x, double light_y, double radius, double alpha);
void menu_ctrl_set_light_img(menu_ctrl *ctrl, char *path);
int menu_ctrl_set_style(menu_ctrl *ctrl, char *background, char *scale, char *indicator,
                        char *def, char *selected, char *activated, char *bgImagePath, int bg_from_time, int draw_scales, int font_bumpmap, int shadow_offset, Uint8 shadow_alpha, char **bg_color_palette, int bg_cp_colors, char **fg_color_palette, int fg_cp_colors);
void menu_ctrl_set_offset(menu_ctrl *ctrl, int x_offset, int y_offset);
void menu_dispose(menu *menu);
int menu_ctrl_draw(menu_ctrl *ctrl);
void hsv_to_rgb(double h, double s, double v, Uint8 *r, Uint8 *g, Uint8 *b);
void color_temp_to_rgb(double temp, Uint8 *r, Uint8 *g, Uint8 *b, double value);

#endif /* MENU_H_ */
