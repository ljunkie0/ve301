#ifndef THEME_H
#define THEME_H

#include "menu/menu_ctrl.h"

typedef struct radio_theme {
    theme *menu_theme;
    char *info_bg_image_path;
    char *volume_bg_image_path;
    char *info_color;
    char *info_scale_color;
} radio_theme;

radio_theme *new_radio_theme(char *info_bg_image_path, char *info_color, char *info_scale_color, char *volume_bg_image_path, theme *menu_theme);
void free_radio_theme(radio_theme *radio_theme);
radio_theme *get_config_theme(const char *theme_name);
void free_theme(radio_theme *rth);

#endif // THEME_H
