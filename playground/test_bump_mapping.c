#include "../src/base/log_contexts.h"
#include "../src/base/logging.h"
#include "../src/menu/menu.h"

int my_sdl_event_callback(menu_ctrl *ctrl, SDL_Event e) {
    if (e.type == SDL_MOUSEMOTION) {
        SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent *) &e;
        menu_ctrl_set_light(ctrl, m->x, m->y, 10, 0);
        //printf("Mouse: %d - %d\n", m->x, m->y);
    }
    return 0;
}

int main(int argc, char **argv) {
    set_log_level(MENU_CTX, 3);

    menu_ctrl *ctrl = menu_ctrl_new(480,
                                    320,
                                    0,
                                    0,
                                    180,
                                    1,
                                    230,
                                    450,
                                    0,
                                    "/home/chris/.ve391/P22Speyside.otf",
                                    28,
                                    -1,
                                    NULL,
                                    NULL);

    menu_ctrl_set_sdl_event_callback(ctrl, &my_sdl_event_callback);

    menu_ctrl_set_light(ctrl, 0, 0, 10, 0);

    theme *t = theme_new();
    t->background_color = "#ffffff";
    t->scale_color = "#000000";
    t->indicator_color = "#ff0000";
    t->default_color = "#000000";
    t->activated_color = "#000000";
    t->selected_color = "#000000";
    t->font_bumpmap = 1;

    menu_ctrl_apply_theme(ctrl, t);
    menu *m = menu_new_root(ctrl, 1, NULL, -1, NULL, -1);
    menu_item *item = menu_item_new(m, "Test", NULL, NULL, -1, NULL, -1, NULL, NULL, -1);

    while (menu_ctrl_loop(ctrl)) {
    }

    //menu_ctrl_dispose(ctrl);
}
