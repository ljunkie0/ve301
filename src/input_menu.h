#ifndef INPUT_MENU_H
#define INPUT_MENU_H

typedef void input_menu_ok_action(menu *menu, char *input);
menu *input_menu_new (menu_ctrl *ctrl, char *font, int font_size, input_menu_ok_action *ok_action);


#endif // INPUT_MENU_H
