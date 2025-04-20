#include "menucontrol.h"
#include "../base.h"
#include "../sdl_util.h"

#define FONT_DEFAULT "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

MenuControl::MenuControl(int w, int x_offset, int y_offset, int radius_labels, int draw_scales, int radius_scales_start, int radius_scales_end, double angle_offset, const char *font, int font_size, int font_size2 /*,
                         item_action *action, menu_callback *call_back*/ ) {

    this->w = w;
    this->x_offset = x_offset;
    this->y_offset = y_offset;
    this->radius_labels = radius_labels;
    this->draw_scales = draw_scales;
    this->radius_scales_start = radius_scales_start;
    this->radius_scales_end = radius_scales_end;
    this->angle_offset = angle_offset;

    if (!init_SDL()) {
        log_error(MENU_CTX, "Failed to initialize SDL\n");
        return;
    }

    this->font_size = font_size;
    if (font_size2 > 0) {
        this->font_size2 = font_size2;
    } else {
        this->font_size2 = font_size - 8;
    }

    if (font) {
        log_config(MENU_CTX, "Trying to open font %s\n", font);
        this->font = TTF_OpenFont(font, font_size);
        this->font2 = TTF_OpenFont(font, this->font_size2);
    }

    if (!this->font) {
        log_error(MENU_CTX, "Failed to load font %s: %s. Trying %s\n", font, SDL_GetError(), FONT_DEFAULT);
        this->font = TTF_OpenFont(FONT_DEFAULT, font_size);
        this->font2 = TTF_OpenFont(FONT_DEFAULT, this->font_size2);
        if (!this->font) {
            log_error(MENU_CTX, "Failed to load font %s: %s. Trying /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf.\n", FONT_DEFAULT, SDL_GetError());
            this->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", font_size);
            this->font2 = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", this->font_size2);
            if (!this->font) {
                log_error(MENU_CTX, "Failed to load font: %s\n", SDL_GetError());
            }
        }
    } else {
        log_config(MENU_CTX, "Font %s successfully opened\n", font);
    }

}
