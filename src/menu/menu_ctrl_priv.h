#ifndef MENU_CTRL_PRIV_H
#define MENU_CTRL_PRIV_H

#include "menu_ctrl.h"

#include <SDL2/SDL_ttf.h>

struct menu_ctrl {
    int w;
    int h;
    int x_offset;
    int y_offset;
    SDL_Point center;
    double light_x;
    double light_y;
    double angle_offset;
    menu **root;
    int n_roots;
    menu *current;
    menu *active;
    int warp_speed; // 0-10
    double offset;
    int no_of_scales;
    int segments_per_item; /* The number of segments left and right that belong to a menu item */
    int n_o_items_on_scale; /* The number of items on the scale at the same time */
    int draw_scales;
    int radius_labels;
    int radius_scales_start;
    int radius_scales_end;
    TTF_Font *font;
    int font_size;
    TTF_Font *font2;
    int font_size2;
    int mouse_control;
    SDL_Color *scale_color; /* The color of the scales */
    SDL_Color *default_color; /* The default foregound color */
    SDL_Color *selected_color; /* The foreground color of the selected item */
    SDL_Color *activated_color; /* The foreground color of the active item (currently unsupported) */
    SDL_Color *background_color; /* The background color */
    SDL_Color *indicator_color; /* The color of the vertical indicator line */
    SDL_Color *indicator_color_light; /* The color of the vertical indicator line */
    SDL_Color *indicator_color_dark; /* The color of the vertical indicator line */
    int font_bumpmap; /* Apply bumpmap effect to font */
    int shadow_offset; /* The offset of the drop shadow (0 -> no shadow) */
    Uint8 shadow_alpha; /* The alpha of the drop shadow */
    Uint8 indicator_alpha;
    SDL_Texture *bg_image;
    double bg_segment;
    theme *theme;
    SDL_Texture *light_texture;
    int light_img_x;
    int light_img_y;
    menu_callback *call_back;
    item_action *action;
    int warping;
    SDL_Window *display;
    SDL_Renderer *renderer;
    int loop;
    /**
     * User data
    **/
    unsigned int disposed;
    void *object;
};


int menu_ctrl_draw_indicator(menu_ctrl *ctrl, double xc, double yc, double angle);
int do_clear(menu_ctrl *ctrl, double angle, SDL_Color *background_color, SDL_Texture *bg_image);
int menu_ctrl_apply_light(menu_ctrl *ctrl);

#endif // MENU_CTRL_PRIV_H
