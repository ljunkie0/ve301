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

#define _GNU_SOURCE

#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../sdl_util.h"
#include <SDL2/SDL2_rotozoom.h>
#include <execinfo.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#ifdef RASPBERRY
#include "../rotaryencoder.h"
#endif

#include "menu_item_priv.h"
#include "menu_menu_priv.h"
#include "menu_ctrl_priv.h"

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

#define USE_UNICODE

static SDL_Color black = { 0, 0, 0, 255 };
static SDL_Color white = { 255, 255, 255, 255 };

void __menu_turn_left(menu *m, int redraw);
void __menu_turn_right(menu *m, int redraw);
void menu_fade_out(menu *menu_frm, menu *menu_to);
int menu_ctrl_process_events(menu_ctrl *ctrl);
int menu_draw(menu *m, int clear, int render);

int menu_action(menu_event evt, menu_ctrl *ctrl, menu *menu) {

    if (menu && menu->action) {
        return ((item_action*)menu->action)(evt, menu, NULL);
    } else if (ctrl->current && ctrl->current->action) {
        return ctrl->current->action(evt, ctrl->current, NULL);
    } else if (ctrl->action) {
        return ((item_action*)ctrl->action)(evt, ctrl->current, NULL);
    }

    return 0;

}

void __menu_warp_sleep(menu_ctrl *ctrl) {
    if (ctrl->warp_speed < 10) {
        usleep((10 - ctrl->warp_speed) * 1000);
    }
}

void menu_item_warp_to(menu_item *item) {
    log_debug(MENU_CTX,
              "menu_item_warp_to: menu_item->menu->ctrl->object = %p->%p->%p->%p\n",
              item,
              item->menu,
              item->menu->ctrl,
              item->menu->ctrl->object);
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
            __menu_turn_right(m,1);
            menu_ctrl_process_events(ctrl);
            __menu_warp_sleep(ctrl);
        }
    } else {
        while (item->id != m->current_id && ctrl->warping) {
            __menu_turn_left(m,1);
            menu_ctrl_process_events(ctrl);
            __menu_warp_sleep(ctrl);
        }
    }

    // Lock in
    while (m->segment < 0 && ctrl->warping) {
        __menu_turn_right(m,1);
        menu_ctrl_process_events(ctrl);
        __menu_warp_sleep(ctrl);
    }

    while (m->segment > 0 && ctrl->warping) {
        __menu_turn_left(m,1);
        menu_ctrl_process_events(ctrl);
        __menu_warp_sleep(ctrl);
    }

    ctrl->warping = 0;

    log_debug(MENU_CTX, "menu_item_warp_to: ctrl->object = %p->%p\n", ctrl, ctrl->object);
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

void menu_update_cnt_rad(menu *m, SDL_Point center, int radius, int recursive) {
    for (int i = 0; i <= m->max_id; i++) {
        if (m->item[i]) {
            if (recursive && m->item[i]->sub_menu) {
                menu_update_cnt_rad((menu *) m->item[i]->sub_menu, center, radius, recursive);
            } else {
                menu_item_update_cnt_rad(m->item[i],center,radius);
            }
        }
    }
}

void menu_fade_out(menu *menu_frm, menu *menu_to) {
    menu_ctrl *ctrl = (menu_ctrl *) menu_frm->ctrl;
    ctrl->warping = 1;
    double R_frm = menu_frm ? menu_frm->radius_scales_end : 0.0;
    double r_frm = menu_frm ? menu_frm->radius_labels : 0.0;
    double d_frm = R_frm - r_frm;

    double R_to = r_frm;
    double r_to = 0;

    int s = 20;
    double ds = (double) s;
    double inv_ds = 1 / ds;

    int i = 0;
    for (i = 0; i <= s; i++) {

        double x = (double) i;
        double x_1 = ds - x;

        if (menu_frm) {
            menu_frm->radius_labels = to_int(inv_ds * (x * R_frm + x_1 * r_frm));
            menu_update_cnt_rad(menu_frm, ctrl->center, menu_frm->radius_labels, 0);
            menu_frm->radius_scales_end = to_int(inv_ds * (x * (R_frm + d_frm) + x_1 * R_frm));
            menu_frm->dirty = 1;
            menu_draw(menu_frm, 1, 0);
        }
        menu_to->radius_labels = to_int(inv_ds * (x * r_frm + x_1 * r_to));
        menu_update_cnt_rad(menu_to, ctrl->center, menu_to->radius_labels, 0);
        menu_to->radius_scales_end = to_int(inv_ds * (x * R_frm + x_1 * R_to));
        menu_to->dirty = 1;
        menu_draw(menu_to, 0, 1);
    }

    menu_frm->radius_labels = ctrl->radius_labels;
    menu_update_cnt_rad(menu_frm, ctrl->center, menu_frm->radius_labels, 0);
    menu_to->radius_labels = ctrl->radius_labels;
    menu_update_cnt_rad(menu_to, ctrl->center, menu_to->radius_labels, 0);

    menu_to->dirty = 1;
    menu_draw(menu_to, 1, 1);
    if (!menu_to->transient) {
        ctrl->active = menu_to;
    }
    ctrl->current = menu_to;
}

void menu_fade_in(menu *menu_frm, menu *menu_to) {
    menu_ctrl *ctrl = (menu_ctrl *) menu_frm->ctrl;

    ctrl->warping = 1;
    double R_frm = menu_frm->radius_scales_end;
    double r_frm = menu_frm->radius_labels;
    double d_frm = R_frm - r_frm;

    double r_to = R_frm;
    double R_to = R_frm + d_frm;

    int s = 20;
    double ds = (double) s;
    double inv_ds = 1 / ds;

    int i = 0;
    for (i = 0; i <= s; i++) {

        double x = (double) i;
        double x_1 = ds - x;

        menu_frm->radius_labels = to_int(inv_ds * x_1 * r_frm);
        menu_update_cnt_rad(menu_frm, ctrl->center, menu_frm->radius_labels, 0);
        menu_frm->radius_scales_end = to_int(inv_ds * (x * r_frm + x_1 * R_frm));
        menu_to->radius_labels = to_int(inv_ds * (x * r_frm + x_1 * r_to));
        menu_update_cnt_rad(menu_to, ctrl->center, menu_to->radius_labels, 0);
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
    log_debug(MENU_CTX, "Open sub menu %s\n", item->label);
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
    } else {
        log_warning(MENU_CTX, "Menu already opened\n");
    }
    return 1;
}

void __menu_turn(menu *m, int direction, int redraw) {
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

    if (redraw) {
        menu_ctrl_draw(m->ctrl);
    }

}

void menu_turn_right(menu *m) {
    __menu_turn_right(m, 1);
}

void menu_turn_left(menu *m) {
    __menu_turn_left(m, 1);
}

void __menu_turn_right(menu *m, int redraw) {
    __menu_turn(m,1,redraw);
}

void __menu_turn_left(menu *m, int redraw) {
    __menu_turn(m,-1,redraw);
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

void menu_ctrl_set_active(menu_ctrl *ctrl, menu *active) {
    ctrl->active = active;
}

void menu_ctrl_set_offset(menu_ctrl *ctrl, int x_offset, int y_offset) {
    ctrl->x_offset = x_offset;
    ctrl->y_offset = y_offset;
    SDL_Point center;

    center.x = ctrl->x_offset + ctrl->w / 2.0;
    center.y = ctrl->y_offset + 0.5 * ctrl->offset * ctrl->w;

    ctrl->center = center;

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_update_cnt_rad(ctrl->root[r],ctrl->center,ctrl->radius_labels, 1);
    }

}

menu *menu_ctrl_get_root(menu_ctrl *ctrl) {
    return ctrl->root[0];
}

void menu_ctrl_draw_indicator(menu_ctrl *ctrl, double xc, double yc, double angle) {
    double a = - M_PI * ctrl->angle_offset / 180.0 - M_PI_2;

    double cos_a = cos(a);
    double sin_a = sin(a);


    double fx1 = xc + ctrl->w * cos_a;
    double fy1 = yc - ctrl->w * sin_a;
    double fx2 = xc - ctrl->w * cos_a;
    double fy2 = yc + ctrl->w * sin_a;

    SDL_SetRenderDrawBlendMode(
            ctrl->renderer,
            SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
            ctrl->renderer,
            ctrl->indicator_color->r,
            ctrl->indicator_color->g,
            ctrl->indicator_color->b,
            ctrl->indicator_color->a);

    SDL_RenderDrawLineF(
            ctrl->renderer,
            fx1,
            fy1,
            fx2,
            fy2);

    SDL_RenderDrawLineF(
            ctrl->renderer,
            fx1-1.0,
            fy1,
            fx2-1.0,
            fy2);

    SDL_RenderDrawLineF(
            ctrl->renderer,
            fx1+1.0,
            fy1,
            fx2+1.0,
            fy2);

    SDL_SetRenderDrawColor(
            ctrl->renderer,
            ctrl->indicator_color_light->r,
            ctrl->indicator_color_light->g,
            ctrl->indicator_color_light->b,
            ctrl->indicator_color->a * 180 / 255);

    SDL_RenderDrawLineF(
            ctrl->renderer,
            fx1-2.0,
            fy1,
            fx2-2.0,
            fy2);

    SDL_SetRenderDrawColor(
            ctrl->renderer,
            ctrl->indicator_color_dark->r,
            ctrl->indicator_color_dark->g,
            ctrl->indicator_color_dark->b,
            ctrl->indicator_color->a * 180 / 255);

    SDL_RenderDrawLineF(
            ctrl->renderer,
            fx1+2.0,
            fy1,
            fx2+2.0,
            fy2);
}

void menu_ctrl_apply_light(menu_ctrl *ctrl) {
    if (ctrl->light_texture) {
        double w = ctrl->w;
        double h = ctrl->h;
        double xo = ctrl->center.x - 0.5 * ctrl->w;
        double yo = 0;
        const SDL_Rect dst_rect = {xo + ctrl->light_img_x, yo + ctrl->light_img_y, w, h};
        SDL_RenderCopy(ctrl->renderer,ctrl->light_texture,NULL,&dst_rect);
    }
}

void menu_ctrl_set_radii(menu_ctrl *ctrl, int radius_labels, int radius_scales_start, int radius_scales_end) {

    if (ctrl->root) {
        for (int r = 0; r < ctrl->n_roots; r++) {
            menu_set_radius(ctrl->root[r], radius_labels, radius_scales_start, radius_scales_end);
        }
    }

}

int menu_ctrl_get_n_o_items_on_scale(menu_ctrl *ctrl) {
    return ctrl->n_o_items_on_scale;
}

menu *menu_ctrl_get_active(menu_ctrl *ctrl) {
    return ctrl->active;
}

int menu_ctrl_draw(menu_ctrl *ctrl) {
    if (ctrl->current) {
        return menu_draw(ctrl->current, 1, 1);
    }

    return 0;

}

int menu_ctrl_set_style(menu_ctrl *ctrl, char *background, char *scale, char *indicator,
        char *def, char *selected, char *activated, char *bgImagePath,
        int draw_scales, int font_bumpmap, int shadow_offset,
        Uint8 shadow_alpha, char **bg_color_palette, int bg_cp_colors,
        char **fg_color_palette, int fg_cp_colors
        ) {
    log_config(MENU_CTX, "START: menu_ctrl_set_style (");
    log_config(MENU_CTX, "background -> %s", background);
    log_config(MENU_CTX, ", scale -> %s", scale);
    log_config(MENU_CTX, ", def -> %s", def);
    log_config(MENU_CTX, ", selected -> %s", selected);
    log_config(MENU_CTX, ", activated -> %s", activated);
    log_config(MENU_CTX, ", indicator -> %s", indicator);
    log_config(MENU_CTX, ", bgImagePath -> %s", bgImagePath);
    log_config(MENU_CTX, ")\n");

    free_and_set_null((void **) &ctrl->background_color);
    ctrl->background_color = html_to_color(background);

    free_and_set_null((void **) &ctrl->scale_color);
    ctrl->scale_color = html_to_color(scale);

    free_and_set_null((void **) &ctrl->default_color);
    ctrl->default_color = html_to_color(def);

    free_and_set_null((void **) &ctrl->selected_color);
    ctrl->selected_color = html_to_color(selected);

    free_and_set_null((void **) &ctrl->activated_color);
    ctrl->activated_color = html_to_color(activated);

    free_and_set_null((void **) &ctrl->indicator_color);
    ctrl->indicator_color = html_to_color_and_alpha(indicator,&(ctrl->indicator_alpha));

    free_and_set_null((void **) &ctrl->indicator_color_light);
    ctrl->indicator_color_light = color_between(ctrl->indicator_color, &white, 0.85);

    free_and_set_null((void **) &ctrl->indicator_color_dark);
    ctrl->indicator_color_dark = color_between(ctrl->indicator_color, &black, 0.85);

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

    for (int r = 0; r < ctrl->n_roots; r++) {
        menu_rebuild_glyphs(ctrl->root[r]);
    }

    int draw_res = menu_ctrl_draw(ctrl);
    log_config(MENU_CTX, "END: menu_ctrl_set_style\n");
    return draw_res;

}

int menu_ctrl_apply_theme(menu_ctrl *ctrl, theme *theme) {
    return menu_ctrl_set_style(ctrl, theme->background_color, theme->scale_color,
            theme->indicator_color, theme->default_color, theme->selected_color,
            theme->activated_color, theme->bg_image_path, 1, theme->font_bumpmap, theme->shadow_offset, theme->shadow_alpha, theme->bg_color_palette, theme->bg_cp_colors, theme->fg_color_palette, theme->fg_cp_colors);
}

void menu_ctrl_set_light(menu_ctrl *ctrl, double light_x, double light_y, double radius, double alpha) {
    if (ctrl->light_texture) {
        SDL_DestroyTexture(ctrl->light_texture);
        free (ctrl->light_texture);
    }
    ctrl->light_x = light_x;
    ctrl->light_y = light_y;

    ctrl->light_texture = new_light_texture(ctrl->renderer, ctrl->w, ctrl->h, light_x, light_y, radius, alpha);

}

void menu_ctrl_set_light_img(menu_ctrl *ctrl, char *path, int x, int y) {
    if (ctrl->light_texture) {
        SDL_DestroyTexture(ctrl->light_texture);
    }

    ctrl->light_img_x = x;
    ctrl->light_img_y = y;
    ctrl->light_texture = IMG_LoadTexture(ctrl->renderer, path);

}

void menu_ctrl_set_warp_speed(menu_ctrl *ctrl, const int warp_speed) {
    ctrl->warp_speed = warp_speed;
    if (ctrl->warp_speed < 0) {
        ctrl->warp_speed = 0;
        return;
    }
    if (ctrl->warp_speed > 10) {
        ctrl->warp_speed = 10;
    }
}

menu_ctrl *menu_ctrl_new(int w, int h, int x_offset, int y_offset, int radius_labels, int draw_scales, int radius_scales_start, int radius_scales_end, double angle_offset, const char *font, int font_size, int font_size2,
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
    ctrl->light_texture = NULL;
    ctrl->warp_speed = 10;

    ctrl->w = w;
    ctrl->h = h > 0 ? h : to_int(0.65 * w);

    ctrl->root = NULL;
    ctrl->n_roots = 0;

    ctrl->object = NULL;

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

    menu_ctrl_set_radii(ctrl,radius_labels,radius_scales_start,radius_scales_end);

    ctrl->call_back = call_back;
    ctrl->action = action;
    ctrl->bg_image = NULL;

    if (!init_SDL()) {
        log_error(MENU_CTX, "Failed to initialize SDL\n");
        menu_ctrl_dispose(ctrl);
        return 0;
    }

    if (font) {
        log_config(MENU_CTX, "Trying to open font %s\n", font);
        ctrl->font = my_OpenTTF_Font(font, font_size);
        ctrl->font2 = my_OpenTTF_Font(font, ctrl->font_size2);
    }

    if (!ctrl->font) {
        log_error(MENU_CTX, "Failed to load font %s: %s. Trying %s\n", font, SDL_GetError(), FONT_DEFAULT);
        ctrl->font = my_OpenTTF_Font(FONT_DEFAULT, font_size);
        ctrl->font2 = my_OpenTTF_Font(FONT_DEFAULT, ctrl->font_size2);
        if (!ctrl->font) {
            log_error(MENU_CTX, "Failed to load font %s: %s. Trying /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf.\n", FONT_DEFAULT, SDL_GetError());
            ctrl->font = my_OpenTTF_Font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", font_size);
            ctrl->font2 = my_OpenTTF_Font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", ctrl->font_size2);
            if (!ctrl->font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
    } else {
        log_config(MENU_CTX, "Font %s successfully opened\n", font);
    }

    log_info(MENU_CTX, "Creating window...");
    ctrl->display = SDL_CreateWindow("VE301",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     ctrl->w,
                                     ctrl->h,
                                     SDL_WINDOW_HIDDEN);
    log_info(MENU_CTX, "Done\n");
    if (ctrl->display) {
        log_info(MENU_CTX, "Creating renderer...");
#ifdef RASPBERRY
        ctrl->renderer = SDL_CreateRenderer(ctrl->display,
                                            1,
                                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
        ctrl->renderer = SDL_CreateRenderer(ctrl->display,
                                            -1,
                                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
        log_info(MENU_CTX, "Done\n");
        SDL_RendererInfo rendererInfo;
        log_info(MENU_CTX, "Getting renderer info...\n");
        SDL_GetRendererInfo(ctrl->renderer, &rendererInfo);
        log_info(MENU_CTX, "SDL chose the following renderer:\n");
        log_info(MENU_CTX,
                 "Renderer: %s software=%d accelerated=%d, presentvsync=%d targettexture=%d\n",
                 rendererInfo.name,
                 (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0,
                 (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0,
                 (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0,
                 (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0);
    }

    if (!ctrl->display) {
        log_error(MENU_CTX, "Failed to create window: %s\n", SDL_GetError());
        menu_ctrl_dispose(ctrl);
        return 0;
    }

#ifdef RASPBERRY
    menu_ctrl_set_style (ctrl, "#08081e", "#c8c8c8", "#ff0000", "#525239", "#c8c864", "#c8c864", 0, 1, 0, 0, 0, NULL, 0, NULL, 0);
#else
    menu_ctrl_set_style(ctrl, "#08081e", "#c8c8c8", "#ff0000", "#525239", "#c8c864",
            "#c8c864",
            0, 1, 0, 0, 0, NULL, 0, NULL, 0);
#endif

    ctrl->loop = 0;

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
            __menu_turn_left (ctrl->current, 0);
            menu_action (TURN_LEFT, ctrl, ctrl->current);
            redraw = 1;
        } else if (he == BUTTON_A_TURNED_RIGHT) {
            __menu_turn_right (ctrl->current, 0);
            menu_action (TURN_RIGHT, ctrl, ctrl->current);
            redraw = 1;
        } else if (he == BUTTON_A_PRESSED) {
            if (ctrl->current && ctrl->current->current_id >= 0 && ctrl->current->item) {
                menu_item *item = ctrl->current->item[ctrl->current->current_id];
                menu_item_action (ACTIVATE, ctrl, item);
                redraw = 1;
            } else {
                menu_action (ACTIVATE, ctrl, ctrl->current);
                redraw = 1;
            }
        } else if (he == BUTTON_A_HOLD) {
            open_parent_menu (ctrl);
        } else if (he == BUTTON_B_TURNED_LEFT) {
            menu_action(TURN_LEFT_1, ctrl, ctrl->current);
        } else if (he == BUTTON_B_TURNED_RIGHT) {
            menu_action(TURN_RIGHT_1, ctrl, ctrl->current);
        } else if (he == BUTTON_B_PRESSED) {
            if (ctrl->current && ctrl->current->current_id >= 0 && ctrl->current->item) {
                menu_item *item = ctrl->current->item[ctrl->current->current_id];
                menu_item_action (ACTIVATE_1, ctrl, item);
                redraw = 1;
            } else {
                menu_action (ACTIVATE_1, ctrl, ctrl->current);
                redraw = 1;
            }
        } else if (he == BUTTON_B_HOLD) {
            menu_action (HOLD_1, ctrl, ctrl->current);
        }
        he = next_event ();
    }

#endif
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            log_debug(MENU_CTX, "Caught event %03x\n", e.type);
            if (e.type == SDL_QUIT) {
                return -1;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                SDL_MouseButtonEvent *b = (SDL_MouseButtonEvent *) &e;
                if (b->state == SDL_RELEASED) {
                    if (b->button == 1) {
                        if (ctrl->current->current_id >= 0) {
                            menu_item *item =
                                ctrl->current->item[ctrl->current->current_id];
                            menu_item_action(ACTIVATE, ctrl, item);
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
                        __menu_turn_left(ctrl->current,0);
                        menu_action(TURN_LEFT, ctrl, ctrl->current);
                        redraw = 1;
                    } else if (b->button == 5) {
                        __menu_turn_right(ctrl->current,0);
                        menu_action(TURN_RIGHT, ctrl, ctrl->current);
                        redraw = 1;
                    }
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                SDL_MouseWheelEvent *w = (SDL_MouseWheelEvent *) &e;
                if (w->y > 0) {
                    __menu_turn_left(ctrl->current,0);
                    menu_action(TURN_LEFT, ctrl, ctrl->current);
                    redraw = 1;
                } else if (w->y < 0) {
                    __menu_turn_right(ctrl->current,0);
                    menu_action(TURN_RIGHT, ctrl, ctrl->current);
                    redraw = 1;
                }
            } else if (e.type == SDL_KEYUP) {
                SDL_KeyboardEvent *k = (SDL_KeyboardEvent *) &e;
                if (k->state == SDL_RELEASED) {
                    if (k->keysym.sym == SDLK_UP) {
                        menu_action(TURN_RIGHT_1, ctrl, ctrl->current);
                    } else if (k->keysym.sym == SDLK_DOWN) {
                        menu_action(TURN_LEFT_1, ctrl, ctrl->current);
                    } else if (k->keysym.sym == SDLK_q) {
                        return -1;
                    }
                }
            }
        }

        return redraw;
}

void menu_ctrl_free(menu_ctrl *ctrl) {
    if (ctrl) {
        log_info(MENU_CTX, "Freeing menu ctrl %p\n", ctrl);

        ctrl->current = NULL;

        if (ctrl->display) {
            SDL_DestroyWindow(ctrl->display);
            ctrl->display = NULL;
        }

        if (ctrl->renderer) {
            SDL_DestroyRenderer(ctrl->renderer);
            ctrl->renderer = NULL;
        }

        if (ctrl->root) {
            for (int r = 0; r < ctrl->n_roots; r++) {
                menu_free(ctrl->root[r]);
                ctrl->root[r] = NULL;
            }
            free(ctrl->root);
            ctrl->root = NULL;
        }

        free_and_set_null((void **) &ctrl->activated_color);
        free_and_set_null((void **) &ctrl->background_color);
        free_and_set_null((void **) &ctrl->default_color);
        free_and_set_null((void **) &ctrl->indicator_color);
        free_and_set_null((void **) &ctrl->indicator_color_dark);
        free_and_set_null((void **) &ctrl->indicator_color_light);
        free_and_set_null((void **) &ctrl->scale_color);
        free_and_set_null((void **) &ctrl->selected_color);
        free_and_set_null((void **) &ctrl->object);

        if (ctrl->font) {
            TTF_CloseFont(ctrl->font);
        }

        if (ctrl->font2) {
            TTF_CloseFont(ctrl->font2);
        }

        free (ctrl);
    }
}

void menu_ctrl_dispose(menu_ctrl *ctrl) {
    log_info(MENU_CTX, "Dispose\n");
    log_info(MENU_CTX, "Cleaning up menu ctrl\n");
    menu_ctrl_free(ctrl);
    log_info(MENU_CTX, "Closing TTF\n");
    TTF_Quit();
    log_info(MENU_CTX, "Closing IMG\n");
    IMG_Quit();
    log_info(MENU_CTX, "Closing SDL\n");
    SDL_Quit();
}

int menu_ctrl_loop(menu_ctrl *ctrl) {
    log_info(MENU_CTX, "START: menu_ctrl_loop\n");
#ifdef RASPBERRY
    setup_encoder ();
#endif

    menu_ctrl_draw(ctrl);

    SDL_ShowWindow(ctrl->display);

    while (1) {
        int res = menu_ctrl_process_events(ctrl);
        log_trace(MENU_CTX, "events result: %d\n", res);
        if (res == -1) {
            break;
        } else if (res == 0) {
            if (ctrl->call_back) {
                ctrl->call_back(ctrl);
            }
            SDL_Delay(20);
        } else {
            menu_ctrl_draw(ctrl);
        }
    }
    log_info(MENU_CTX, "END: menu_ctrl_loop\n");
    return 0;
}
