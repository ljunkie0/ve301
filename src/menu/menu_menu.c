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
#include "../base.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../sdl_util.h"
#include "menu_ctrl_priv.h"
#include "menu_item_priv.h"
#include "menu_menu_priv.h"

int menu_get_max_id(menu *m) {
    return m->max_id;
}

menu_item *menu_get_item(menu *m, int id) {
    return m->item[id];
}

int menu_draw_scale(menu *m, double xc, double yc, double r, double R, double angle, unsigned char alpha, int lines) {

    double a = M_PI * angle / 180.0;
    double cos_a = cos(a);
    double sin_a = sin(a);
    double fx1 = xc + r * cos_a;
    double fy1 = yc - r * sin_a;
    double fx2 = xc + R * cos_a; double fy2 = yc - R * sin_a;

    menu_ctrl *ctrl = m->ctrl;

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
        Uint8 r,g,b;

        if (m->scale_color) {
            r = m->scale_color->r;
            g = m->scale_color->g;
            b = m->scale_color->b;
        } else {
            r = ctrl->scale_color->r;
            g = ctrl->scale_color->g;
            b = ctrl->scale_color->b;
        }

        if (lines == 1) {
            SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ctrl->renderer, r, g, b, alpha/2);
            SDL_RenderDrawLineF(ctrl->renderer, fx1-1, fy1, fx2-1, fy2);
            SDL_RenderDrawLineF(ctrl->renderer, fx1+1, fy1, fx2+1, fy2);
        }
        SDL_SetRenderDrawBlendMode(ctrl->renderer,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ctrl->renderer, r, g, b, alpha);
        SDL_RenderDrawLineF(ctrl->renderer, fx1, fy1, fx2, fy2);
    }

    return 0;
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

        menu_draw_scale(m, xc, yc, fr-20, fR, a, 255, 0);

        a += angle_step;

        int sd;
        for (sd = 0; sd < no_of_mini_scales; sd++) {

            menu_draw_scale(m,xc,yc,fr,fR,a, 255, 0);

            a += angle_step;

        }

    }

    return 0;

}

int menu_wipe(menu *m, double angle) {
    log_debug(MENU_CTX, "START menu_wipe\n");
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
    Uint32 render_start_ticks = SDL_GetTicks();

    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;

    double xc = ctrl->center.x;
    double yc = ctrl->center.y;

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
            while (current_item < 0) {
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

    menu_ctrl_draw_indicator(ctrl, xc, yc, angle);

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

menu *menu_new(
    menu_ctrl *ctrl,
    int lines,
    const char *font,
    int font_size,
    item_action *action,
    char *font_2nd_line,
    int font_size_2nd_line
    ) {

    menu *m = malloc(sizeof(menu));

    m->max_id = -1;
    m->active_id = -1;
    m->current_id = -1;
    m->sticky = 0;
    m->dirty = 0;
    m->user_data = NULL;

    m->radius_labels = ctrl->radius_labels;
    m->radius_scales_start = ctrl->radius_scales_start;
    m->radius_scales_end = ctrl->radius_scales_end;

    m->parent = m;
    m->item = NULL;
    m->segment = 0;
    m->label = NULL;
    m->bg_image = NULL;
    m->scale_color = NULL;
    m->default_color = NULL;
    m->selected_color = NULL;

    m->ctrl = ctrl;

    m->transient = 0;
    m->draw_only_active = 0;
    m->n_o_lines = lines;
    m->n_o_items_on_scale = lines * ctrl->n_o_items_on_scale;
    m->segments_per_item = ctrl->segments_per_item;

    m->font = NULL;
    if (font && font_size > 0) {
        m->font = my_OpenTTF_Font(font,font_size);
    }
    m->font2 = NULL;
    if (font_2nd_line && font_size_2nd_line > 0) {
        m->font2 = my_OpenTTF_Font(font_2nd_line, font_size_2nd_line);
    }

    m->action = action;

    if (!ctrl->current) {
        ctrl->current = m;
    }

    return m;

}

void menu_free(menu *m) {
    if (m) {
        if (m->label) {
            log_config(MENU_CTX, "Freeing menu %s\n", m->label);
        } else {
            log_config(MENU_CTX, "Freeing menu %p\n", m);
        }
        for (int i = 0; i <= m->max_id; i++) {
            menu_item_dispose(m->item[i]);
            m->item[i] = NULL;
        }

        free(m->item);
        free_and_set_null((void **) &m->label);
        free_and_set_null((void **) &m->default_color);
        free_and_set_null((void **) &m->selected_color);
        free_and_set_null((void **) &m->scale_color);
        free_and_set_null((void **) &m->user_data);

        if (m->bg_image) {
            SDL_DestroyTexture(m->bg_image);
        }
        if (m->selected_color) {
            free (m->selected_color);
            m->selected_color = NULL;
        }
        if (m->scale_color) {
            free (m->scale_color);
            m->scale_color = NULL;
        }

        free (m);
    }
}

menu *menu_new_root(
    menu_ctrl *ctrl,
    int lines,
    const char *font,
    int font_size,
    char *font_2nd_line,
    int font_size_2nd_line
    ) {
    menu *m = menu_new(ctrl,lines,font, font_size, NULL, font_2nd_line, font_size_2nd_line);
    ctrl->n_roots = ctrl->n_roots + 1;
    ctrl->root = realloc(ctrl->root,ctrl->n_roots * sizeof(menu *));
    ctrl->root[ctrl->n_roots-1] = m;
    if (!ctrl->current) {
        ctrl->current = m;
    }
    return m;
}

void menu_dispose(menu *menu) {
    if (!menu) {
        return;
    }

    menu_clear(menu);

}

menu *menu_get_parent(menu *menu) {
    return menu->parent;
}

menu_item *menu_add_sub_menu(menu *m, const char *label, menu *sub_menu, item_action *action) {

    menu_item *item = menu_item_new(m, label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;
    sub_menu->parent = m;

    const char *llabel = label ? label : "";
    if ((!sub_menu->label) || strcmp(sub_menu->label, llabel)) {
        if (sub_menu->label) {
            free (sub_menu->label);
            sub_menu->label = NULL;
        }
        sub_menu->label = my_copystr(llabel);
    }

    return item;

}

menu_item *menu_new_sub_menu(menu *m, const char *label, item_action *action) {

    menu *sub_menu = menu_new(m->ctrl, 1, NULL, 0, NULL, NULL, 0);
    menu_item *item = menu_item_new(m, label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, action, NULL, -1);
    item->sub_menu = sub_menu;

    return item;

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

void menu_set_draw_only_active(menu *menu, int draw_only_active) {
    menu->draw_only_active = draw_only_active;
}

void menu_set_radius(menu *m, int radius_labels, int radius_scales_start, int radius_scales_end) {

    if (radius_labels >= 0)
        m->radius_labels = radius_labels;
    if (radius_scales_start >= 0)
        m->radius_scales_start = radius_scales_start;
    if (radius_scales_end >= 0)
        m->radius_scales_end = radius_scales_end;

    int i = 0;
    for (i = 0; i <= m->max_id; i++) {
        if (m->item[i] && m->item[i]->sub_menu) {
            menu_set_radius((menu *) m->item[i]->sub_menu, radius_labels, radius_scales_start, radius_scales_end);
        }
    }
}

void menu_rebuild_glyphs(menu *m) {
    for (int i = 0; i <= m->max_id; i++) {
        if (m->item[i]) {
            if (m->item[i]->sub_menu) {
                menu_rebuild_glyphs((menu *) m->item[i]->sub_menu);
            }
            menu_item_rebuild_glyphs(m->item[i]);
        }
    }
}

int menu_set_colors(menu *m, SDL_Color *default_color, SDL_Color *selected_color, SDL_Color *scale_color) {
    if (m->scale_color) {
        free(m->scale_color);
    }

    if (scale_color) {
        m->scale_color = clone_color(scale_color);
    }

    if (m->default_color) {
        free(m->default_color);
    }

    if (default_color) {
        m->default_color = clone_color(default_color);
    }

    if (m->selected_color) {
        free(m->selected_color);
    }

    if (selected_color) {
        m->selected_color = clone_color(selected_color);
    }

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

void menu_set_no_items_on_scale(menu *m, int n) {
    m->n_o_items_on_scale = n;
}

void menu_set_radius_labels(menu *m, int radius) {
    m->radius_labels = radius;
}

void menu_set_active_id(menu *m, int id) {
    m->active_id = id;
}

char *menu_get_label(menu *m) {
    return m->label;
}

void menu_set_label(menu *m, const char *label) {
    m->label = my_copystr(label);
}

int menu_is_transient(menu *m) {
    return m->transient;
}

void menu_set_transient(menu *m, int transient) {
    m->transient = transient;
}

int menu_is_sticky(menu *m) {
    return m->sticky;
}

void menu_set_segments_per_item(menu *m, int segments) {
    m->segments_per_item = segments;
}

menu_item *menu_get_current_item(menu *m) {
    return m && m->current_id >= 0 ? m->item[m->current_id] : NULL;
}
