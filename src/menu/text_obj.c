/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "text_obj.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LABEL_LENGTH 25
#define M_2_X_PI 6.28318530718
#define VISIBLE_ANGLE 72.0
#define TEXT_OBJ_LABEL_LINE_SPACING 0.8

static int text_obj_read_utf8_char(const char *txt, int *idx, Uint16 *out) {
    unsigned char c = (unsigned char) txt[(*idx)++];
    Uint16 u = (Uint16) c;
    int l = 0;

    if (c > 252) {
        l = 5;
    } else if (c > 248) {
        l = 4;
    } else if (c > 240) {
        l = 3;
    } else if (c > 224) {
        l = 2;
    } else if (c > 192) {
        l = 1;
    }

    if (l > 0) {
        u = c & 0x3f;
        u = (Uint16) (u << 6 * l);

        for (int b = 1; b <= l && txt[*idx]; b++) {
            Uint16 n = (Uint16) (unsigned char) txt[(*idx)++];
            n = n & 0x3f;
            n = (Uint16) (n << 6 * (l - b));
            u = u | n;
        }
    }

    *out = u;
    return 1;
}

static void text_obj_decode_lines(const char *txt, Uint16 **lines, Uint32 *lengths) {
    int line = 0;
    int idx = 0;

    while (txt && txt[idx]) {
        Uint16 u;
        text_obj_read_utf8_char(txt, &idx, &u);
        if (u == '\n') {
            if (line < TEXT_OBJ_MAX_LINES - 1) {
                line++;
            }
            continue;
        }

        if (lengths[line] >= MAX_LABEL_LENGTH) {
            continue;
        }

        lines[line] = realloc(lines[line], (lengths[line] + 1) * sizeof(Uint16));
        lines[line][lengths[line]++] = u;
    }

    for (int i = 0; i < TEXT_OBJ_MAX_LINES; i++) {
        if (lengths[i] > 0) {
            lines[i] = realloc(lines[i], (lengths[i] + 1) * sizeof(Uint16));
            lines[i][lengths[i]] = 0;
        }
    }
}

static void text_obj_free_unicode_lines(Uint16 **lines) {
    for (int i = 0; i < TEXT_OBJ_MAX_LINES; i++) {
        free(lines[i]);
        lines[i] = NULL;
    }
}

static int text_obj_count_lines(const text_obj *obj) {
    int n_lines = 0;

    if (!obj) {
        return 0;
    }

    for (int i = 0; i < TEXT_OBJ_MAX_LINES; i++) {
        if (obj->lines[i].n_glyphs > 0) {
            n_lines = i + 1;
        }
    }

    return n_lines;
}

static int text_obj_line_height(const text_obj *obj) {
    int height = 0;

    if (!obj) {
        return 0;
    }

    for (int i = 0; i < TEXT_OBJ_MAX_LINES; i++) {
        if (obj->lines[i].height > height) {
            height = obj->lines[i].height;
        }
    }

    return height;
}

static int text_obj_label_radius(const text_obj *obj, int base_radius, int label_line) {
    int label_lines = obj->n_lines > 0 ? obj->n_lines : text_obj_count_lines(obj);
    int line_height = text_obj_line_height(obj);
    double offset = 0.0;

    if (label_lines > 1 && line_height > 0) {
        offset = 0.5 * TEXT_OBJ_LABEL_LINE_SPACING * line_height
                 * (label_lines - 1 - 2 * label_line);
    }

    return base_radius + (int) offset;
}

static void text_obj_free_line(text_obj_line *line) {
    if (!line) {
        return;
    }

    if (line->glyphs_objs) {
        for (int g = 0; g < line->n_glyphs; g++) {
            glyph_obj_free(line->glyphs_objs[g]);
        }
        free(line->glyphs_objs);
    }

    line->glyphs_objs = NULL;
    line->n_glyphs = 0;
    line->width = 0;
    line->height = 0;
}

void text_obj_free(text_obj *obj) {
    if (obj) {
        for (int i = 0; i < TEXT_OBJ_MAX_LINES; i++) {
            text_obj_free_line(&obj->lines[i]);
        }
        free(obj);
    }
}

void text_obj_update_cnt_rad(text_obj *obj, SDL_Point center, int radius,
                             int line, int n_lines) {
    if (obj) {
        int base_radius = radius + 0.8 * text_obj_line_height(obj) * (line + 0.5 * (n_lines - 1));

        obj->n_lines = text_obj_count_lines(obj);
        for (int l = 0; l < obj->n_lines; l++) {
            int line_radius = text_obj_label_radius(obj, base_radius, l);
            for (int g = 0; g < obj->lines[l].n_glyphs; g++) {
                glyph_obj_update_cnt_rad(obj->lines[l].glyphs_objs[g], center, line_radius);
            }
        }
    }
}

static int text_obj_build_icon_line(text_obj *t,
                                    SDL_Renderer *renderer,
                                    const char *icon,
                                    SDL_Point center,
                                    int radius,
                                    int bump_map) {
    SDL_Surface *text_surface = IMG_Load(icon);
    if (text_surface == NULL) {
        log_error(MENU_CTX, "Could not create text surface for \"icon=[%s]\": %s\n",
                  icon,
                  IMG_GetError());
        return 0;
    }

    t->lines[0].n_glyphs = 1;
    t->lines[0].glyphs_objs = calloc(1, sizeof(glyph_obj *));
    t->lines[0].width = text_surface->w;
    t->lines[0].height = text_surface->h;

    IMG_Animation *animation = IMG_LoadAnimation(icon);
    if (animation && animation->count > 1) {
        t->lines[0].glyphs_objs[0] = glyph_obj_new_animated(renderer,
                                                            animation->frames,
                                                            animation->count,
                                                            center,
                                                            radius,
                                                            bump_map);
        free(animation->frames);
        free(animation->delays);
        free(animation);
        SDL_FreeSurface(text_surface);
    } else {
        if (animation) {
            IMG_FreeAnimation(animation);
        }
        t->lines[0].glyphs_objs[0] = glyph_obj_new_surface(renderer,
                                                           text_surface,
                                                           center,
                                                           radius,
                                                           bump_map);
    }

    if (!t->lines[0].glyphs_objs[0]) {
        log_error(MENU_CTX, "Could not create glyph object for icon %s\n", icon);
        return 0;
    }

    return 1;
}

static int text_obj_build_text_line(text_obj *t,
                                    int line,
                                    SDL_Renderer *renderer,
                                    Uint16 *unicode_text,
                                    Uint32 n_glyphs,
                                    TTF_Font *font,
                                    SDL_Color fg,
                                    SDL_Point center,
                                    int radius,
                                    int bump_map,
                                    const char *txt) {
    SDL_Surface *text_surface;

    if (n_glyphs == 0 || unicode_text == NULL) {
        return 1;
    }

    text_surface = TTF_RenderUNICODE_Blended(font, unicode_text, fg);
    if (text_surface == NULL) {
        log_error(MENU_CTX,
                  "Could not create glyph surface for %s (line = %d, unicode_length = %d): %s\n",
                  txt,
                  line,
                  n_glyphs,
                  TTF_GetError());
        return 0;
    }

    t->lines[line].n_glyphs = n_glyphs;
    t->lines[line].glyphs_objs = calloc(n_glyphs, sizeof(glyph_obj *));
    t->lines[line].width = text_surface->w;
    t->lines[line].height = text_surface->h;
    log_debug(MENU_CTX, "SDL_FreeSurface(text_surface => %p) (line %d);\n", text_surface, line);
    SDL_FreeSurface(text_surface);

    for (Uint32 i = 0; i < n_glyphs; i++) {
        t->lines[line].glyphs_objs[i]
            = glyph_obj_new(renderer, unicode_text[i], font, fg, center, radius, bump_map);
        if (!t->lines[line].glyphs_objs[i]) {
            log_error(MENU_CTX, "Could not create glyph object for %c\n", unicode_text[i]);
            return 0;
        }
    }

    return 1;
}

text_obj *text_obj_new(SDL_Renderer *renderer,
                       char *txt,
                       char *icon,
                       TTF_Font *font,
                       TTF_Font *font_2nd_line,
                       SDL_Color fg,
                       SDL_Point center,
                       int radius,
                       int line,
                       int n_lines,
                       int light_x,
                       int light_y,
                       int bump_map) {
    (void) light_x;
    (void) light_y;

    if ((txt && strlen(txt) > 0) || (icon && strlen(icon) > 0)) {
        Uint16 *unicode_lines[TEXT_OBJ_MAX_LINES] = {0};
        Uint32 unicode_lengths[TEXT_OBJ_MAX_LINES] = {0};
        text_obj *t = calloc(1, sizeof(text_obj));

        if (txt) {
            text_obj_decode_lines(txt, unicode_lines, unicode_lengths);
        }

        if (icon) {
            free(unicode_lines[TEXT_OBJ_MAX_LINES - 1]);
            for (int i = TEXT_OBJ_MAX_LINES - 1; i > 1; i--) {
                unicode_lines[i] = unicode_lines[i - 1];
                unicode_lengths[i] = unicode_lengths[i - 1];
            }
            unicode_lines[1] = unicode_lines[0];
            unicode_lengths[1] = unicode_lengths[0];
            unicode_lines[0] = NULL;
            unicode_lengths[0] = 0;

            if (!text_obj_build_icon_line(t, renderer, icon, center, radius, bump_map)) {
                text_obj_free_unicode_lines(unicode_lines);
                text_obj_free(t);
                return NULL;
            }
        }

        for (int i = icon ? 1 : 0; i < TEXT_OBJ_MAX_LINES; i++) {
            TTF_Font *line_font = i == 0 ? font : font_2nd_line;
            if (!line_font) {
                line_font = font;
            }
            if (!line_font && unicode_lengths[i] > 0) {
                log_error(MENU_CTX, "Could not render text line %d for %s: no font available\n", i, txt);
                text_obj_free_unicode_lines(unicode_lines);
                text_obj_free(t);
                return NULL;
            }
            if (!text_obj_build_text_line(t,
                                          i,
                                          renderer,
                                          unicode_lines[i],
                                          unicode_lengths[i],
                                          line_font,
                                          fg,
                                          center,
                                          radius,
                                          bump_map,
                                          txt)) {
                text_obj_free_unicode_lines(unicode_lines);
                text_obj_free(t);
                return NULL;
            }
        }

        t->n_lines = text_obj_count_lines(t);
        text_obj_update_cnt_rad(t, center, radius, line, n_lines);
        text_obj_free_unicode_lines(unicode_lines);

        return t;

    } else {
        log_info(MENU_CTX, "No icon and not label provided\n");
    }

    return NULL;
}

static void text_obj_draw_line_shadow(SDL_Renderer *renderer,
                                      text_obj_line *line,
                                      int center_x,
                                      int center_y,
                                      double angle,
                                      double light_x,
                                      double light_y,
                                      int font_bumpmap,
                                      int shadow_offset,
                                      int shadow_alpha) {
    double advance = -0.5 * line->width;

    for (int c = 0; c < line->n_glyphs; c++) {
        glyph_obj *glyph_obj = line->glyphs_objs[c];
        double crc;
        double a;
        SDL_Texture *texture;

        glyph_obj_animation_update(glyph_obj);

        crc = M_2_X_PI * glyph_obj->radius;
        a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w) / crc;

        if (a != glyph_obj->current_angle) {
            glyph_obj_update_bumpmap_texture(renderer, glyph_obj, center_x,
                                             center_y, a, light_x, light_y);
        }

        texture = font_bumpmap ? glyph_obj->bumpmap_overlay : glyph_obj->texture;

        if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {
            Uint8 orig_a, orig_r, orig_g, orig_b;
            SDL_Rect shadow_dst_rec;
            SDL_GetTextureAlphaMod(texture, &orig_a);
            SDL_GetTextureColorMod(texture, &orig_r, &orig_g, &orig_b);
            SDL_SetTextureColorMod(texture, 0, 0, 0);

            shadow_dst_rec.w = glyph_obj->dst_rect->w;
            shadow_dst_rec.h = glyph_obj->dst_rect->h;

            for (int so = shadow_offset; so > 0; so--) {
                int sa = (shadow_offset - so + 1) * shadow_alpha / (shadow_offset);
                SDL_SetTextureAlphaMod(texture, sa);
                shadow_dst_rec.x = glyph_obj->dst_rect->x + so * glyph_obj->shadow_dx;
                shadow_dst_rec.y = glyph_obj->dst_rect->y + so * glyph_obj->shadow_dy;
                SDL_RenderCopyEx(renderer, texture, NULL, &shadow_dst_rec, a,
                                 glyph_obj->rot_center, SDL_FLIP_NONE);
            }

            SDL_SetTextureAlphaMod(texture, orig_a);
            SDL_SetTextureColorMod(texture, orig_r, orig_g, orig_b);
        }

        advance += glyph_obj->advance;
    }
}

static void text_obj_draw_line(SDL_Renderer *renderer,
                               text_obj_line *line,
                               int center_x,
                               int center_y,
                               double angle,
                               double light_x,
                               double light_y,
                               int font_bumpmap) {
    double advance = -0.5 * line->width;

    for (int c = 0; c < line->n_glyphs; c++) {
        glyph_obj *glyph_obj = line->glyphs_objs[c];
        double crc = M_2_X_PI * glyph_obj->radius;
        double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w) / crc;

        if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {
            if (font_bumpmap) {
                SDL_Texture *texture;
                if (a != glyph_obj->current_angle) {
                    glyph_obj_update_bumpmap_texture(renderer, glyph_obj, center_x,
                                                     center_y, a, light_x, light_y);
                }

                texture = glyph_obj->bumpmap_overlay;
                log_trace(MENU_CTX, "texture: %p\n", texture);
                SDL_RenderCopyEx(renderer, texture, NULL, glyph_obj->dst_rect, a,
                                 glyph_obj->rot_center, SDL_FLIP_NONE);
            } else {
                SDL_RenderCopyEx(renderer, glyph_obj->texture, NULL,
                                 glyph_obj->dst_rect, a, glyph_obj->rot_center,
                                 SDL_FLIP_NONE);
            }
        } else {
            log_debug(MENU_CTX, "angle %f not in visible range of %f\n", angle,
                      VISIBLE_ANGLE);
        }

        glyph_obj->current_angle = a;
        advance += glyph_obj->advance;
    }
}

void text_obj_draw(SDL_Renderer *renderer, SDL_Texture *target, text_obj *label,
                   int radius, int center_x, int center_y, double angle,
                   double light_x, double light_y, int font_bumpmap,
                   int shadow_offset, int shadow_alpha) {
    double circumference = M_2_X_PI * radius;

    log_debug(MENU_CTX,
              "text_obj_draw: angle: %f, radius: %d, circumference: %f\n",
              angle,
              radius,
              circumference);
    if (radius == 0) {
        log_debug(MENU_CTX, "text_obj_draw: radius = 0, returning\n");
        return;
    }

    if (!label) {
        return;
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer, target);
    }

    if (shadow_offset > 0) {
        for (int l = 0; l < label->n_lines; l++) {
            text_obj_draw_line_shadow(renderer,
                                      &label->lines[l],
                                      center_x,
                                      center_y,
                                      angle,
                                      light_x,
                                      light_y,
                                      font_bumpmap,
                                      shadow_offset,
                                      shadow_alpha);
        }
    } else {
        log_debug(MENU_CTX, "No shadow\n");
    }

    for (int l = 0; l < label->n_lines; l++) {
        text_obj_draw_line(renderer,
                           &label->lines[l],
                           center_x,
                           center_y,
                           angle,
                           light_x,
                           light_y,
                           font_bumpmap);
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, target, NULL, NULL);
    }
}
