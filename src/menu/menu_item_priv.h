#ifndef MENU_ITEM_PRIV_H
#define MENU_ITEM_PRIV_H

#include "menu_item.h"
#include "text_obj.h"
#include <SDL2/SDL_ttf.h>

typedef struct menu_item {
    int id;
    Uint16 *unicode_label;
    Uint16 *unicode_label2;
    char *label;
    int num_label_chars;
    int num_label_chars2;
    int w;
    int h;
    int visible;
    menu *sub_menu;
    TTF_Font *font;
    TTF_Font *font2;
    menu *menu;
    int line; // The line (0 = default, >0 = above, <0 = below)

    text_obj *label_default;
    text_obj *label_active;
    text_obj *label_current;

    item_action *action;
    int w2;
    int h2;
    char *icon;
    /**
     * User data
    **/
    int object_type;
    const void *object;
} menu_item;

void menu_item_update_cnt_rad(menu_item *item, SDL_Point center, int radius);
int menu_item_draw(menu_item *item, menu_item_state st, double angle);
void menu_item_rebuild_glyphs(menu_item *item);
int menu_item_action(menu_event evt, menu_ctrl *ctrl, menu_item *item);

#endif // MENU_ITEM_PRIV_H
