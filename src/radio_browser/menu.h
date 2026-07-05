#ifndef RADIO_BROWSER_MENU_H
#define RADIO_BROWSER_MENU_H
#include "../audio/player.h"
#include "../menu/menu_ctrl.h"
typedef void __radio_app_touch_activity(int);

menu *radio_browser_menu_init(menu_ctrl *ctrl,
                              __radio_app_touch_activity *radio_app_touch_activity,
                              const char *server,
                              const char *user_agent,
                              const char *country_code,
                              int station_limit,
                              int category_limit,
                              int language_limit,
                              player *radio_player);

void radio_browser_menu_close();

#endif // RADIO_BROWSER_MENU_H
