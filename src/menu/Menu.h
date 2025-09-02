#ifndef MENUO_H
#define MENUO_H
#include "MenuCtrl.h"
#include "MenuItem.h"

class MenuCtrl;

class Menu {

private:
    MenuCtrl *ctrl;
    menu *handle;
    friend class MenuItem;
public:
    Menu(MenuCtrl *ctrl,
	 int lines,
	 const char *font,
	 int fontSize,
	 item_action *action,
	 char *font2ndLine,
	 int fontSize2ndLine);
    ~Menu();
    int setBackgroundImage(char *bgImagePath);
    int setColors(SDL_Color *defaultColor, SDL_Color *selectedColor, SDL_Color *scaleColor);
    //menu_item *menu_new_sub_menu(menu *m, const char*label, item_action *action);
    //menu_item *menu_add_sub_menu(menu *m, const char*label, menu *sub_menu, item_action *action);
    int open();
    int clear();
    void turnLeft();
    void turnRight();

};

#endif // MENUO_H
