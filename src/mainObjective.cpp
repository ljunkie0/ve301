#include "menu/MenuItem.h"

int menu_call_back(menu_ctrl *m_ptr) {
	return 0;
}

int main(int argc, char **argv) {
	MenuCtrl *ctrl = new MenuCtrl(
			/* w => */ 640,
			/* xOffset => */ 0,
			/* yOffset => */ 0,
			/* radiusLabels => */ 300,
			/* drawScales => */ 1,
			/* radiusScalesStart */ 400,
			/* radiusScalesEnd */ 500,
			/* angleOffset */ 0.0,
			/* font */ (const char *) 0,
			/* fontSize */ 24,
			/* fontSize2 */ 24,
			/* action */ (item_action *) 0,
			/* callBack */ &menu_call_back 
	);
	Menu *m = new Menu(
			/* ctrl => */ ctrl,
			/* lines => */ 1,
			/* font => */ (const char *) 0,
			/* fontSize => */ -1,
			/* action => */ (item_action *) 0,
			/* font2ndLine => */ (char *) 0,
			/* fontSize2ndLine => */ -1
	);

	const char *spotify_logo="/home/chris/.ve301/VE301_Logo_zeichn.png";

	MenuItem *i1 = new MenuItem(/* m => */ m, /* label => */ "Hello World");
	MenuItem *i2 = new MenuItem(/* m => */ m, /* label => */ NULL, spotify_logo);
    i2->setVisible(0);

	ctrl->setBackgroundColor(100,100,100);	
	ctrl->enableFontBumpmap();

	ctrl->loop();
	return 0;
}
