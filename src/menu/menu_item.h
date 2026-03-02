#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include "menu_menu.h"

typedef enum { ACTIVE, SELECTED, DEFAULT } menu_item_state;

typedef struct menu_item menu_item;
typedef int item_action(menu_event, menu *, menu_item *);

/**
 * Menu Items
 **/
menu_item *menu_item_new(menu *m,
                         const char *label,
                         const char *icon,
                         const void *object,
                         int object_type,
                         const char *font,
                         int font_size,
                         item_action *action,
                         char *font_2nd_line,
                         int font_size_2nd_line);
int menu_item_dispose(menu_item *item);
void menu_item_free(menu_item *item);
int menu_item_is_sub_menu(menu_item *item);
void menu_item_activate(menu_item *item);
void menu_item_warp_to(menu_item *item);
void menu_item_show(menu_item *item);
menu_item *menu_item_update_label(menu_item *item, const char *label);
char *menu_item_get_label(menu_item *item);
menu_item *menu_item_update_icon(menu_item *item, const char *icon);
void menu_item_set_visible(menu_item *item, const int visible);
int menu_item_get_visible(menu_item *item);
void menu_item_set_object_type(menu_item *item, int object_type);
int menu_item_get_object_type(menu_item *item);
int menu_item_is_object_type(menu_item *item, int object_type);
void menu_item_set_sub_menu(menu_item *item, menu *sub_menu);
menu *menu_item_get_sub_menu(menu_item *item);
const void *menu_item_get_object(menu_item *item);
void menu_item_free_object(menu_item *item);
void menu_item_set_object(menu_item *item, void *object);
int menu_item_get_id(menu_item *item);
menu *menu_item_get_menu(menu_item *item);

#endif // MENU_ITEM_H
