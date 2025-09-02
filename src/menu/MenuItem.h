#ifndef MENUITEM_H
#define MENUITEM_H
#include "menu.h"
#include "MenuCtrl.h"

class Menu;

class MenuItem {

private:
    Menu *m;
    menu_item *handle;
    void init(Menu *m,
         const char *label,
	 const char *iconPath,
         const void *object,
         int objectType,
         const char *font,
         int fontSize,
         item_action *action,
         char *font2ndLine,
         int fontSize2ndLine);
public:
    MenuItem(Menu *m,
	     const char *label,
	     const char *iconPath,
	     const void *object,
	     int objectType,
	     const char *font,
	     int fontSize,
	     item_action *action,
	     char *font2ndLine,
	     int fontSize2ndLine);
    MenuItem(Menu *m,
	     const char *label);
    MenuItem(Menu *m,
	     const char *label,
	     const char *iconPath);
    ~MenuItem();
    void activate();
    void warpTo();
    void show();
    void updateLabel(const char *label);
    void updateIcon(const char *icon);
    void setVisible(int visible);
    int isVisible();

};

#endif // MENUITEM_H
