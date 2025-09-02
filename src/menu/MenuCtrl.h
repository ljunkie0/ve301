#ifndef MENUCTRL_H
#define MENUCTRL_H
#include "menu.h"
#include "Menu.h"

class MenuCtrl {

private:
    menu_ctrl *ctrl;
    friend class Menu;
public:
    MenuCtrl(int w,
          int xOffset,
	  int yOffset,
	  int radiusLabels,
	  int drawScales,
	  int radiusScalesStart,
	  int radiusScalesEnd,
	  double angleOffset,
	  const char *font,
	  int fontSize,
	  int fontSize2,
          item_action *action,
	  menu_callback *callBack);
    ~MenuCtrl();
    void quit();
    void loop();
    void setRadii(int radiusLabels, int radiusScalesStart, int radiusScalesEnd);
    int applyTheme(theme *theme);
    int setBackgroundColor(Uint8 r, Uint8 g, Uint8 b);
    int setDefaultColor(Uint8 r, Uint8 g, Uint8 b);
    int setActiveColor(Uint8 r, Uint8 g, Uint8 b);
    int setSelectedColor(Uint8 r, Uint8 g, Uint8 b);
    void setLight(double x, double y, double radius, double alpha);
    void setLightImage(char *path, int x, int y);
    void enableFontBumpmap();
    void disableFontBumpmap();
};

#endif // MENUCTRL_H
