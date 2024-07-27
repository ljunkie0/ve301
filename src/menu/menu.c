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

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <execinfo.h>
#include <SDL2/SDL2_rotozoom.h>
#include <unistd.h>
#include "../base.h"
#include "../sdl_util.h"
#include "text_obj.h"
#include "menu.h"

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

#define MAX_LABEL_LENGTH 25
#define FONT_DEFAULT "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#define BOLD_FONT_DEFAULT "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"

#define Y_OFFSET 35

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132
#endif

#define USE_UNICODE

static SDL_Color black = { 0, 0, 0, 255 };
static SDL_Color white = { 255, 255, 255, 255 };

static int current_hour = -1;

/* For FPS measuring */
static Uint32 render_start_ticks;

/**
* Menu item state:
* SELECTED: Currently the one under the indicator
* ACTIVE: the radio station e.g. that is currently played
* DEFAULT: all others
**/
typedef enum {
    ACTIVE, SELECTED, DEFAULT
} menu_item_state;

void menu_turn_left(menu *m);
void menu_turn_right(menu *m);
void menu_fade_out(menu *menu_frm, menu *menu_to);
int menu_ctrl_draw_scale(menu_ctrl *ctrl, double xc, double yc, double r, double R, double angle, unsigned char alpha, int lines);
int menu_ctrl_clear(menu_ctrl *ctrl, double angle);
int menu_ctrl_process_events(menu_ctrl *ctrl);
int menu_draw(menu *m, int clear, int render);
int menu_ctrl_apply_light(menu_ctrl *ctrl);

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

    log_config(MENU_CTX, "menu_item_draw: label = %s, line = %d, state = %d, angle = %f\n", item->label, item->line, st, angle);

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

        text_obj_draw(item->menu->ctrl->renderer,NULL,label,item->menu->radius_labels,item->menu->ctrl->center.x,item->menu->ctrl->center.y,angle,item->line,item->menu->ctrl->light_x,item->menu->ctrl->light_y, item->menu->ctrl->font_bumpmap, item->menu->ctrl->shadow_offset, item->menu->ctrl->shadow_alpha);

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
    return item->sub_menu != NULL;
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
    item->label_default = text_obj_new(renderer,item->label,font,font2,m->default_color != NULL ? *m->default_color : *m->ctrl->default_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_current = text_obj_new(renderer,item->label,font,font2,m->selected_color != NULL ? *m->selected_color : *m->ctrl->selected_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_active = text_obj_new(renderer,item->label,font,font2,m->default_color != NULL ? *m->default_color : *m->ctrl->activated_color,m->ctrl->center,m->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);

}

int menu_item_set_label(menu_item *item, const char *label) {

    if (!item->label || strcmp(item->label, label)) {
        const char *llabel = (label ? label : "");
        if (item->label) {
            free(item->label);
            item->label = NULL;
        }

        item->label = my_copystr(llabel);

        menu_item_rebuild_glyphs(item);

        item->menu->dirty = 1;

    }

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
    item->sub_menu = NULL;
    item->object_type = object_type;
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

    if (item->sub_menu) {
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
            menu_ctrl_process_events(ctrl);
        }
    } else {
        while (item->id != m->current_id && ctrl->warping) {
            menu_turn_left(m);
            menu_ctrl_process_events(ctrl);
        }
    }

    // Lock in
    while (m->segment < 0 && ctrl->warping) {
        menu_turn_right(m);
        menu_ctrl_process_events(ctrl);
    }

    while (m->segment > 0 && ctrl->warping) {
        menu_turn_left(m);
        menu_ctrl_process_events(ctrl);
    }

    ctrl->warping = 0;
    //menu_ctrl_draw(ctrl);
    log_config(MENU_CTX, "menu_item_warp_to: ctrl->object = %p->%p\n", ctrl, ctrl->object);

}

int do_clear(menu_ctrl *ctrl, double angle, SDL_Color *background_color, SDL_Texture *bg_image) {
    if (background_color) {
        SDL_SetRenderDrawColor(ctrl->renderer,background_color->r,
                               background_color->g,
                               background_color->b, 255);
        if (SDL_RenderClear(ctrl->renderer) < 0) {
            log_error(MENU_CTX, "Failed to render background: %s\n", SDL_GetError());
            return 0;
        }
    }

    if (!bg_image) {
        bg_image = ctrl->bg_image;
    }

    if (bg_image) {
        int w, h;
        SDL_QueryTexture(bg_image,NULL,NULL,&w,&h);
        double xo = ctrl->center.x - 0.5 * w;
        double yo = ctrl->center.y - 0.5 * h;
        const SDL_FRect dst_rect = {xo, yo, w, h};
        double xc = 0.5 * dst_rect.w;
        double yc = 0.5 * dst_rect.h;
        const SDL_FPoint center = {xc,yc};

        if (SDL_RenderCopyExF(ctrl->renderer, bg_image, NULL, &dst_rect, angle, &center, SDL_FLIP_NONE) == -1) {
            log_error(MENU_CTX, "Failed to render background: %s\n", SDL_GetError());
            return 0;
        }
    }

    return 1;
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
            menu_frm->dirty = 1;
            menu_draw(menu_frm, 1, 0);
        }
        menu_to->radius_labels = to_int(inv_ds * (x * r_frm + x_1 * r_to));
        menu_to->radius_scales_end = to_int(inv_ds * (x * R_frm + x_1 * R_to));
        menu_to->dirty = 1;
        menu_draw(menu_to, 0, 1);
    }
    menu_to->dirty = 1;
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
        menu_frm->dirty = 1;
        menu_draw(menu_frm, 1, 0);
        menu_to->dirty = 1;
        menu_draw(menu_to, 0, 1);
    }
    menu_to->dirty = 1;
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

int menu_draw_scales(menu *m, double xc, double yc, double angle) {

    menu_ctrl *ctrl = m->ctrl;

    int no_of_mini_scales = 2;

    double a = angle;
    double angle_step = 360.0 / ((no_of_mini_scales+1) * ctrl->no_of_scales);
    double fr = m->radius_scales_start;
    double fR = m->radius_scales_end;

    /* Draw the scales */

    for (int s = 0; s < ctrl->no_of_scales; s++) {

        menu_ctrl_draw_scale(ctrl, xc, yc, fr-20, fR, a, 255, 0);

        a += angle_step;

        int sd;
        for (sd = 0; sd < no_of_mini_scales; sd++) {

            menu_ctrl_draw_scale(ctrl,xc,yc,fr,fR,a, 255, 0);

            a += angle_step;

        }

    }

    return 0;

}

int menu_wipe(menu *m, double angle) {
    return do_clear(m->ctrl, angle, m->ctrl->background_color, m->bg_image);
}

int menu_draw(menu *m, int clear, int render) {


    if (!m) {
        return 0;
    }

    if (!m->dirty) {
        return 0;
    }

    m->dirty = 0;

    log_debug(MENU_CTX, "START: menu_draw\n");
    render_start_ticks = SDL_GetTicks();

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

    double angle = ctrl->angle_offset + m->segment * 360.0 / (m->n_o_items_on_scale*(2.0*m->segments_per_item+1));
    log_debug(MENU_CTX,"segment: %f, angle: %f\n", m->segment, angle);
    if (clear) {
        double bg_angle = ctrl->angle_offset + ctrl->bg_segment * 360.0 / (m->n_o_items_on_scale*(2.0*m->segments_per_item+1));
        menu_wipe(m,bg_angle);
    }

    if (ctrl->draw_scales) {
        menu_draw_scales(m, xc, yc, angle);
    }

    if (m->max_id >= 0) {

        int i;
        int count_drawn_items = (ctrl->warping && !m->draw_only_active) ? 0.5 * m->n_o_items_on_scale : 0;

        double item_angle_steps = 360 / m->n_o_items_on_scale;

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


            double item_angle = angle + i * item_angle_steps;

            if (item_angle > 360.0) {
                item_angle -= 360.0;
            }

            if (item_angle < - 360.0) {
                item_angle += 360.0;
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

menu *menu_new(menu_ctrl *ctrl, int lines) {

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
    m->bg_image = NULL;
    m->default_color = NULL;
    m->selected_color = NULL;

    m->ctrl = ctrl;

    m->transient = 0;
    m->draw_only_active = 0;
    m->n_o_lines = lines;
    m->n_o_items_on_scale = lines * ctrl->n_o_items_on_scale;
    m->segments_per_item = ctrl->segments_per_item;

    return m;

}

menu *menu_new_root(menu_ctrl *ctrl, int lines) {
    menu *m = menu_new(ctrl,lines);
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

menu_item *menu_add_sub_menu(menu *m, const char *label, menu *sub_menu, item_action *action) {

    menu_item *item = menu_item_new(m, label, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;
    sub_menu->parent = m;

    return item;

}

menu_item *menu_new_sub_menu(menu *m, const char *label, item_action *action) {

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

    menu_item *item = menu_item_new(m, label, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;

    return item;

}

void menu_turn(menu *m, int direction) {
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    ctrl->warping = 1;
    unsigned int total_n_o_segments = m->n_o_items_on_scale * (2.0*m->segments_per_item+1);
    m->segment = m->segment + direction;
    m->dirty = 1;
    if (m->segment > m->segments_per_item) {
        m->segment = -m->segments_per_item;
        m->current_id = m->current_id - 1;
        if (m->current_id < 0) {
            m->current_id = m->max_id;
        }
    } else if (m->segment < -m->segments_per_item) {
        m->segment = m->segments_per_item;
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
        if (m->item[i] && m->item[i]->sub_menu) {
            menu_set_radius((menu *) m->item[i]->sub_menu, radius_labels, radius_scales_start, radius_scales_end);
        }
    }

}

void menu_update_cnt_rad(menu *m, SDL_Point center, int radius) {
    for (int i = 0; i < m->max_id; i++) {
        if (m->item[i]) {
            if (m->item[i]->sub_menu) {
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
            if (m->item[i]->sub_menu) {
                menu_rebuild_glyphs((menu *) m->item[i]->sub_menu);
            } else {
                menu_item_rebuild_glyphs(m->item[i]);
            }
        }
    }
}

int menu_set_colors(menu *m, SDL_Color *default_color, SDL_Color *selected_color) {
    m->default_color = default_color;
    m->selected_color = selected_color;
    menu_rebuild_glyphs(m);

    return 1;

}

int menu_set_bg_image(menu *m, char *bgImagePath) {

    if (m->bg_image) {
        SDL_DestroyTexture(m->bg_image);
        m->bg_image = NULL;
    }

    if (bgImagePath) {
        m->bg_image = IMG_LoadTexture(m->ctrl->renderer,bgImagePath);
        if (!m->bg_image) {
            log_error(MENU_CTX, "Could not load background image %s: %s\n", bgImagePath, SDL_GetError());
        }
    } else {
        m->bg_image = NULL;
    }

    return 1;

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
        if(item->sub_menu) {

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

    /**
     * Decide whether we have to draw
     */
    int doDraw = 1;
    if (fx1 > ctrl->w && fx2 > ctrl->w) {
        doDraw = 0;
    } else if (fx1 < 0 && fx2 < 0) {
        doDraw = 0;
    } else if (fy1 > ctrl->h && fx2 > ctrl->h) {
        doDraw = 0;
    } else if (fy1 < 0 && fx2 < 0) {
        doDraw = 0;
    }

    if (doDraw) {
        if (lines == 1) {
            SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ctrl->renderer, ctrl->scale_color->r, ctrl->scale_color->g, ctrl->scale_color->b, alpha/2);
            SDL_RenderDrawLineF(ctrl->renderer, fx1-1, fy1, fx2-1, fy2);
            SDL_RenderDrawLineF(ctrl->renderer, fx1+1, fy1, fx2+1, fy2);
        }
        SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ctrl->renderer, ctrl->scale_color->r, ctrl->scale_color->g, ctrl->scale_color->b, alpha);
        SDL_RenderDrawLineF(ctrl->renderer, fx1, fy1, fx2, fy2);
    }

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
    do_clear(ctrl, angle, ctrl->background_color, ctrl->bg_image);
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

    ctrl->light_texture = IMG_LoadTexture(ctrl->renderer, path);

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
    ctrl->no_of_scales = 36;
    ctrl->n_o_items_on_scale = 4;
    ctrl->segments_per_item = 3;

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
    root->segments_per_item = ctrl->segments_per_item;

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

    if (!init_SDL()) {
        log_error(MENU_CTX, "Failed to initialize SDL\n");
        return 0;
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

    log_info(MENU_CTX, "Creating window...");
    ctrl->display = SDL_CreateWindow("VE301", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ctrl->w, ctrl->h, 0);
    log_info(MENU_CTX, "Done\n");
    if (ctrl->display) {
        log_info(MENU_CTX, "Creating renderer...");
#ifdef RASPBERRY
        ctrl->renderer = SDL_CreateRenderer(ctrl->display, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
        ctrl->renderer = SDL_CreateRenderer(ctrl->display, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
        log_info(MENU_CTX, "Done\n");
        SDL_RendererInfo rendererInfo;
        log_info(MENU_CTX, "Getting renderer info...\n");
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
            if (item->sub_menu) {
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
        if (m->default_color) {
            free(m->default_color);
        }
        if (m->selected_color) {
            free(m->selected_color);
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
