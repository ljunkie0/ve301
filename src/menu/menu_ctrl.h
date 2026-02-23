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

#ifndef MENU_CTRL_H_
#define MENU_CTRL_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNKNOWN_OBJECT_TYPE -1
#define OBJECT_TYPE_ACTION -2

typedef enum {
    TURN_LEFT,
    TURN_RIGHT,
    ACTIVATE,
    HOLD,
    TURN_LEFT_1,
    TURN_RIGHT_1,
    ACTIVATE_1,
    HOLD_1,
    DISPOSE,
    CLOSE
} menu_event;

typedef struct theme
{
    char *background_color;
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
    u_int8_t shadow_alpha;
} theme;

typedef struct menu menu;
typedef struct menu_item menu_item;
typedef struct menu_ctrl menu_ctrl;
typedef int menu_callback(menu_ctrl *ctrl);
typedef int item_action(menu_event, menu *, menu_item *);

void menu_ctrl_quit(menu_ctrl *ctrl);
void menu_ctrl_dispose(menu_ctrl *ctrl);
menu_ctrl *menu_ctrl_new(int w,
                         int h,
                         int x_offset,
                         int y_offset,
                         int radius_labels,
                         int draw_scales,
                         int radius_scales_start,
                         int radius_scales_end,
                         double angle_offset,
                         const char *font,
                         int font_size,
                         int font_size2,
                         item_action *action,
                         menu_callback *call_back);
void menu_ctrl_enable_mouse(menu_ctrl *ctrl, int mouse_control);
int menu_ctrl_loop(menu_ctrl *ctrl);
void menu_ctrl_set_radii(menu_ctrl *ctrl,
                         int radius_labels,
                         int radius_scales_start,
                         int radius_scales_end);
int menu_ctrl_apply_theme(menu_ctrl *ctrl, theme *theme);
int menu_ctrl_set_bg_color_rgb(menu_ctrl *ctrl, u_int8_t r, u_int8_t g, u_int8_t b);
int menu_ctrl_set_default_color_rgb(menu_ctrl *ctrl, u_int8_t r, u_int8_t g, u_int8_t b);
int menu_ctrl_set_active_color_rgb(menu_ctrl *ctrl, u_int8_t r, u_int8_t g, u_int8_t b);
int menu_ctrl_set_selected_color_rgb(menu_ctrl *ctrl, u_int8_t r, u_int8_t g, u_int8_t b);
void menu_ctrl_set_light(
    menu_ctrl *ctrl, double light_x, double light_y, double radius, double alpha);
void menu_ctrl_set_light_img(menu_ctrl *ctrl, char *path, int x, int y);
void menu_ctrl_set_offset(menu_ctrl *ctrl, int x_offset, int y_offset);
void menu_ctrl_set_angle_offset(menu_ctrl *ctrl, double a);
void menu_ctrl_set_warp_speed(menu_ctrl *ctrl, int warp_speed);
void menu_ctrl_set_active(menu_ctrl *ctrl, menu *active);
void menu_dispose(menu *menu);
int menu_ctrl_draw(menu_ctrl *ctrl);
void hsv_to_rgb(double h, double s, double v, u_int8_t *r, u_int8_t *g, u_int8_t *b);
void color_temp_to_rgb(double temp, u_int8_t *r, u_int8_t *g, u_int8_t *b, double value);
int menu_ctrl_get_n_o_items_on_scale(menu_ctrl *ctrl);
menu *menu_ctrl_get_active(menu_ctrl *ctrl);
menu *menu_ctrl_get_root(menu_ctrl *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* MENU_H_ */
