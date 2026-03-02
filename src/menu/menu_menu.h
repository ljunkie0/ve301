#ifndef MENU_MENU_H
#define MENU_MENU_H

#include "menu_ctrl.h"
#include <SDL2/SDL_pixels.h>

typedef struct menu_item menu_item;
typedef struct menu menu;
typedef int item_action(menu_event, menu *, menu_item *);

menu *menu_new(menu_ctrl *ctrl,
               int lines,
               const char *font,
               int font_size,
               item_action *action,
               char *font_2nd_line,
               int font_size_2nd_line);
menu *menu_new_root(menu_ctrl *ctrl,
                    int lines,
                    const char *font,
                    int font_size,
                    char *font_2nd_line,
                    int font_size_2nd_line);
menu_item *menu_new_sub_menu(menu *m, const char *label, item_action *action);
menu_item *menu_add_sub_menu(menu *m, const char *label, menu *sub_menu, item_action *action);
int menu_open_sub_menu(menu_ctrl *ctrl, menu_item *item);
int menu_get_max_id(menu *m);
menu_item *menu_get_item(menu *m, int id);
int menu_open(menu *m);
int menu_clear(menu *m);
void menu_turn_left(menu *m);
void menu_turn_right(menu *m);
void menu_free(menu *m);
menu_ctrl *menu_get_ctrl(menu *m);
menu *menu_get_parent(menu *m);
void menu_set_active_id(menu *m, int id);
char *menu_get_label(menu *m);
int menu_is_transient(menu *m);
void menu_set_transient(menu *m, int transient);
int menu_is_sticky(menu *m);
menu_item *menu_get_current_item(menu *m);
int menu_get_current_id(menu *m);
int menu_set_bg_image(menu *m, char *bgImagePath);
int menu_set_colors(menu *m,
                    SDL_Color *default_color,
                    SDL_Color *selected_color,
                    SDL_Color *scale_color);
void menu_set_no_items_on_scale(menu *m, int n);
void menu_set_radius_labels(menu *m, int radius);
void menu_set_segments_per_item(menu *m, int segments);
void menu_set_draw_only_active(menu *menu, int draw_only_active);
void menu_set_label(menu *m, const char *label);

#endif // MENU_H
