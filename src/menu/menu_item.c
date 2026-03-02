#include "../base.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../sdl_util.h"
#include "menu_ctrl_priv.h"
#include "menu_item_priv.h"
#include "menu_menu_priv.h"
#include "text_obj.h"

int menu_item_action(menu_event evt, menu_ctrl *ctrl, menu_item *item) {

    if (item) {
        int result = 1;
        if (item->action) {
            result |= ((item_action*)item->action)(evt, item->menu, item);
        } else if (item->menu->action) {
            result |= ((item_action*)item->menu->action)(evt, item->menu, item);
        } else if (ctrl->current && ctrl->current->action) {
            result |= ctrl->current->action(evt, ctrl->current, item);
        } else if (ctrl->action) {
            result |= ((item_action*)ctrl->action)(evt, ctrl->current, item);
        }

        if (item->sub_menu && evt == ACTIVATE) {
            result |= menu_open_sub_menu(ctrl, item);
        }

    } else {

        if (ctrl->current && ctrl->current->action) {
            return ctrl->current->action(evt, ctrl->current, item);
        } else if (ctrl->action) {
            return ((item_action*)ctrl->action)(evt, ctrl->current, item);
        }
    }

    return 0;

}

void menu_item_update_cnt_rad(menu_item *item, SDL_Point center, int radius) {

    int lines = 1;
    if (item->num_label_chars2 > 0) {
        lines = 2;
    }

    int line;
    for (line = 0; line < lines; line++) {

        text_obj_update_cnt_rad(item->label_default, center, radius, item->line, item->menu->n_o_lines);
        text_obj_update_cnt_rad(item->label_active, center, radius, item->line, item->menu->n_o_lines);
        text_obj_update_cnt_rad(item->label_current, center, radius, item->line, item->menu->n_o_lines);

    }

}

int menu_item_draw(menu_item *item, menu_item_state st, double angle) {

    if (!item->visible) {
        return 0;
    }

    log_debug(MENU_CTX,
              "menu_item_draw: label = %s, line = %d, state = %d, angle = %f\n",
              item->label,
              item->line,
              st,
              angle);

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

        if (label) {
            text_obj_draw(item->menu->ctrl->renderer,NULL,label,item->menu->radius_labels,item->menu->ctrl->center.x,item->menu->ctrl->center.y,angle,item->menu->ctrl->light_x,item->menu->ctrl->light_y, item->menu->ctrl->font_bumpmap, item->menu->ctrl->shadow_offset, item->menu->ctrl->shadow_alpha);
        } else {
            log_info(MENU_CTX, "No label, no drawing\n");
        }

    }

    return 1;

}

const void *menu_item_get_object(menu_item *item) {
    return item->object;
}

void menu_item_free_object(menu_item *item) {
    if (item->object) {
        free_and_set_null((void **) &item->object);
    }
}

void menu_item_set_object(menu_item *item, void *object) {
    item->object = object;
}

void menu_item_set_visible(menu_item *item, const int visible) {
    item->visible = visible;
}

int menu_item_get_visible(menu_item *item) {
    return item->visible;
}

char *menu_item_get_label(menu_item *i) {
    return i->label;
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
    sub_menu->parent = item->menu;
}

int menu_item_get_object_type(menu_item *item) {
    return item->object_type;
}

void menu_item_set_object_type(menu_item *item, int object_type) {
    item->object_type = object_type;
}

int menu_item_is_object_type(menu_item *item, int object_type) {
    return item->object_type == object_type;
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
        font = m->font;
    }
    if (!font) {
        font = m->ctrl->font;
    }

    TTF_Font *font2 = item->font2;
    if (!font2) {
        font2 = m->font2;
    }
    if (!font2) {
        font2 = m->ctrl->font2;
    }

    SDL_Renderer *renderer = m->ctrl->renderer;
    item->label_default = text_obj_new(renderer,item->label,item->icon,font,font2,m->default_color != NULL ? *m->default_color : *m->ctrl->default_color,m->ctrl->center,m->ctrl->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_current = text_obj_new(renderer,item->label,item->icon,font,font2,m->selected_color != NULL ? *m->selected_color : *m->ctrl->selected_color,m->ctrl->center,m->ctrl->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);
    item->label_active = text_obj_new(renderer,item->label,item->icon,font,font2,m->default_color != NULL ? *m->default_color : *m->ctrl->activated_color,m->ctrl->center,m->ctrl->radius_labels,item->line, m->n_o_lines, item->menu->ctrl->light_x, item->menu->ctrl->light_y);

}

int menu_item_set_label(menu_item *item, const char *label) {

    const char *llabel = label ? label : "";

    if ((!item->label) || strcmp(item->label, llabel)) {
        if (item->label) {
            free (item->label);
            item->label = NULL;
        }

        item->label = my_copystr(llabel);

        menu_item_rebuild_glyphs(item);

        item->menu->dirty = 1;

    }

    return 1;
}

int menu_item_set_icon(menu_item *item, const char *icon) {

    if (my_strcmp(item->icon, icon)) {
        if (item->icon) {
            free (item->icon);
            item->icon = NULL;
        }

        item->icon = icon ? my_copystr(icon) : NULL;

        menu_item_rebuild_glyphs(item);

        item->menu->dirty = 1;

    }

    return 1;

}

menu_item *menu_item_new(menu *m, const char *label, const char *icon, const void *object, int object_type,
                         const char *font, int font_size, item_action *action, char *font_2nd_line, int font_size_2nd_line) {

    menu_item *item = malloc(sizeof(menu_item));
    item->unicode_label = NULL;
    item->unicode_label2 = NULL;
    item->label = NULL;
    item->object = object;
    item->sub_menu = NULL;
    item->icon = NULL;
    item->object_type = object_type;
    item->menu = m;
    item->action = action;
    item->visible = 1;
    m->max_id++;
    item->id = m->max_id;

    if (m->current_id < 0) {
        m->current_id = 0;
    }

    item->line = (item->id % m->n_o_lines) + 1 - m->n_o_lines;

    if (m->item) {
        m->item = realloc(m->item, (Uint32) (m->max_id + 1) * sizeof(menu_item *));
    } else {
        m->item = malloc((Uint32) (m->max_id + 1) * sizeof(menu_item *));
    }
    m->item[m->max_id] = item;
    item->font = NULL;
    item->font2 = NULL;
    item->num_label_chars = 0;
    item->num_label_chars2 = 0;
    item->label_active = NULL;
    item->label_current = NULL;
    item->label_default = NULL;

    /* Initialize fonts */

    if (font || font_size > 0) {
        TTF_Font *dflt_font = my_OpenTTF_Font(font, font_size);
        if (!dflt_font) {
            log_error(MENU_CTX, "Failed to load font: %s. Trying fixed font\n", SDL_GetError());
            dflt_font = my_OpenTTF_Font("fixed", font_size);
            if (!dflt_font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
        if (dflt_font) {
            item->font = dflt_font;
        }
    }

    if (font_2nd_line || font_size_2nd_line > 0) {
        TTF_Font *dflt_font = my_OpenTTF_Font(font_2nd_line, font_size_2nd_line);
        if (!dflt_font) {
            log_error(MENU_CTX, "Failed to load font: %s. Trying fixed font\n", SDL_GetError());
            dflt_font = my_OpenTTF_Font("fixed", font_size_2nd_line);
            if (!dflt_font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
        if (dflt_font) {
            item->font2 = dflt_font;
        }
    }

    menu_item_set_label(item, label);
    menu_item_set_icon(item, icon);

    return item;

}

void menu_item_free(menu_item *item) {
    if (item) {
        log_config(MENU_CTX, "Freeing menu item %s\n", item->label);

        free_and_set_null((void **) &item->unicode_label);
        free_and_set_null((void **) &item->unicode_label2);
        free_and_set_null((void **) &item->label);
        free_and_set_null((void **) &item->object);

        text_obj_free(item->label_active);
        text_obj_free(item->label_current);
        text_obj_free(item->label_default);

        if (item->font) {
            TTF_CloseFont(item->font);
        }

        if (item->font2) {
            TTF_CloseFont(item->font2);
        }

        if (item->sub_menu) {
            menu_free(item->sub_menu);
        }

        free_and_set_null((void **) &item->icon);

        free(item);
    }
}

int menu_item_dispose(menu_item *item) {
    log_debug(MENU_CTX, "Start dispose_menu_item\n");

    menu *m = (menu *) item->menu;

    if (!m) {
        log_error(MENU_CTX, "Item %s has no menu!\n", item->unicode_label);
        return 1;
    }

    menu_item_action(DISPOSE, m->ctrl, item);

    menu_item_free(item);

    return 1;
}

menu_item *menu_item_update_label(menu_item *item, const char *label) {
    log_debug(MENU_CTX, "menu_item_update_label(item -> %p, label -> %s)\n", item, label);
    menu *m = (menu *) item->menu;
    menu_item_set_label(item, label);
    menu_ctrl_draw(m->ctrl);
    return item;
}

menu_item *menu_item_update_icon(menu_item *item, const char *icon) {
    log_debug(MENU_CTX, "menu_item_update_icon(item -> %p, icon -> %s)\n", item, icon);
    menu *m = (menu *) item->menu;
    menu_item_set_icon(item, icon);
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
