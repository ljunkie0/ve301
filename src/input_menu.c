#include "menu/menu.h"
#include "base.h"
#include "input_menu.h"

static const int input_menu_n_of_characters = 64;

static const char input_menu_alphabet_items[64][7] = {
    {'A', 0, 0, 0, 0, 0, 0},
    {'B', 0, 0, 0, 0, 0, 0},
    {'C', 0, 0, 0, 0, 0, 0},
    {'D', 0, 0, 0, 0, 0, 0},
    {'E', 0, 0, 0, 0, 0, 0},
    {'F', 0, 0, 0, 0, 0, 0},
    {'G', 0, 0, 0, 0, 0, 0},
    {'H', 0, 0, 0, 0, 0, 0},
    {'I', 0, 0, 0, 0, 0, 0},
    {'J', 0, 0, 0, 0, 0, 0},
    {'K', 0, 0, 0, 0, 0, 0},
    {'L', 0, 0, 0, 0, 0, 0},
    {'M', 0, 0, 0, 0, 0, 0},
    {'N', 0, 0, 0, 0, 0, 0},
    {'O', 0, 0, 0, 0, 0, 0},
    {'P', 0, 0, 0, 0, 0, 0},
    {'Q', 0, 0, 0, 0, 0, 0},
    {'R', 0, 0, 0, 0, 0, 0},
    {'S', 0, 0, 0, 0, 0, 0},
    {'T', 0, 0, 0, 0, 0, 0},
    {'U', 0, 0, 0, 0, 0, 0},
    {'V', 0, 0, 0, 0, 0, 0},
    {'W', 0, 0, 0, 0, 0, 0},
    {'X', 0, 0, 0, 0, 0, 0},
    {'Y', 0, 0, 0, 0, 0, 0},
    {'Z', 0, 0, 0, 0, 0, 0},
    {'a', 0, 0, 0, 0, 0, 0},
    {'b', 0, 0, 0, 0, 0, 0},
    {'c', 0, 0, 0, 0, 0, 0},
    {'d', 0, 0, 0, 0, 0, 0},
    {'e', 0, 0, 0, 0, 0, 0},
    {'f', 0, 0, 0, 0, 0, 0},
    {'g', 0, 0, 0, 0, 0, 0},
    {'h', 0, 0, 0, 0, 0, 0},
    {'i', 0, 0, 0, 0, 0, 0},
    {'j', 0, 0, 0, 0, 0, 0},
    {'k', 0, 0, 0, 0, 0, 0},
    {'l', 0, 0, 0, 0, 0, 0},
    {'m', 0, 0, 0, 0, 0, 0},
    {'n', 0, 0, 0, 0, 0, 0},
    {'o', 0, 0, 0, 0, 0, 0},
    {'p', 0, 0, 0, 0, 0, 0},
    {'q', 0, 0, 0, 0, 0, 0},
    {'r', 0, 0, 0, 0, 0, 0},
    {'s', 0, 0, 0, 0, 0, 0},
    {'t', 0, 0, 0, 0, 0, 0},
    {'u', 0, 0, 0, 0, 0, 0},
    {'v', 0, 0, 0, 0, 0, 0},
    {'w', 0, 0, 0, 0, 0, 0},
    {'x', 0, 0, 0, 0, 0, 0},
    {'y', 0, 0, 0, 0, 0, 0},
    {'z', 0, 0, 0, 0, 0, 0},
    {'1', 0, 0, 0, 0, 0, 0},
    {'2', 0, 0, 0, 0, 0, 0},
    {'3', 0, 0, 0, 0, 0, 0},
    {'4', 0, 0, 0, 0, 0, 0},
    {'5', 0, 0, 0, 0, 0, 0},
    {'6', 0, 0, 0, 0, 0, 0},
    {'7', 0, 0, 0, 0, 0, 0},
    {'8', 0, 0, 0, 0, 0, 0},
    {'9', 0, 0, 0, 0, 0, 0},
    {'0',  0,   0,  0, 0, 0, 0},
    {'D', 'E', 'L', 0, 0, 0, 0},
    {'O', 'K',  0,  0, 0, 0, 0}
};

static const int input_menu_del_idx = 62;
static const int input_menu_ok_idx = 63;

char *input_menu_txt = NULL;

input_menu_ok_action *__ok_action;

int input_menu_item_action(menu_event evt, menu *m, menu_item *item) {
    if (evt == ACTIVATE && item && item->menu == m) {
        if (item) {
            if (item->id == input_menu_del_idx) {
                if (input_menu_txt[0] != 0) {
                    int l = strlen(input_menu_txt);
                    if (l == 1) {
                        // if we delete the last character we set input_menu_txt to NULL;
                        input_menu_txt[0] = 0;
                        menu_item_update_label(item,input_menu_alphabet_items[item->id]);
                    } else {
                        char *txt_new = malloc(l*sizeof(char));
                        for (int c = 0; c < l-1; c++) {
                            txt_new[c] = input_menu_txt[c];
                        }
                        txt_new[l-1] = 0;
                        free(input_menu_txt);
                        input_menu_txt = txt_new;
                        char *lbl1 = my_catstr(input_menu_txt,"\n");
                        char *lbl2 = my_catstr(lbl1, input_menu_alphabet_items[item->id]);
                        menu_item_update_label(item,lbl2);
                        free(lbl1);
                        free(lbl2);
                    }
                }
            } else if (item->id == input_menu_ok_idx) {
                if (__ok_action) {
                    __ok_action(m, input_menu_txt);
                }
            } else {
                char *txt_new = my_catstr(input_menu_txt, input_menu_alphabet_items[item->id]);
                free(input_menu_txt);
                input_menu_txt = txt_new;
                char *lbl1 = my_catstr(input_menu_txt,"\n");
                char *lbl2 = my_catstr(lbl1, input_menu_alphabet_items[item->id]);
                menu_item_update_label(item,lbl2);
                free(lbl1);
                free(lbl2);
            }
        }
    } else if (evt == TURN_LEFT) {
        // current_id increases
        if (input_menu_txt[0] != 0) {
            int last_id = m->current_id - 1;
            if (last_id < 0) {
                last_id = input_menu_n_of_characters - 1;
            }

            // menu_item *last_item = m->item[last_id];
            // menu_item_update_label(last_item, input_menu_alphabet_items[last_id]);

            char *lbl1 = my_catstr(input_menu_txt,"\n");
            char *lbl2 = my_catstr(lbl1, m->item[m->current_id]->label);
            menu_item_update_label(m->item[m->current_id],lbl2);
            free(lbl1);
            free(lbl2);
        }
    } else if (evt == TURN_RIGHT) {
        // current_id decreases
        if (input_menu_txt[0] != 0) {
            int last_id = m->current_id + 1;
            if (last_id == input_menu_n_of_characters) {
                last_id = 0;
            }
            menu_item *last_item = m->item[last_id];
            menu_item_update_label(last_item, input_menu_alphabet_items[last_id]);

            char *lbl1 = my_catstr(input_menu_txt,"\n");
            char *lbl2 = my_catstr(lbl1, m->item[m->current_id]->label);
            menu_item_update_label(m->item[m->current_id],lbl2);
            free(lbl1);
            free(lbl2);
        }
    }

    return 1;

}

menu *input_menu_new (menu_ctrl *ctrl, char *font, int font_size, input_menu_ok_action *ok_action) {

    input_menu_txt = malloc(sizeof(char));
    input_menu_txt[0] = 0;

    menu *my_menu = menu_new(ctrl, 1, font, font_size, &input_menu_item_action, font, font_size);
    my_menu->n_o_items_on_scale = input_menu_n_of_characters;
    my_menu->segments_per_item = 0;
    my_menu->current_id = 0;
    my_menu->draw_only_active = 1;
    __ok_action = ok_action;

    for (int c = 0; c < input_menu_n_of_characters; c++) {
        const char *lbl = input_menu_alphabet_items[c];
        menu_item_new(my_menu, lbl, NULL, OBJECT_TYPE_ACTION, NULL, 0, &input_menu_item_action, NULL, 0);
    }

    return my_menu;

}
