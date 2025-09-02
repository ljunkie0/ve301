#include <SDL2/SDL_image.h>
#include "../base.h"
#include "text_obj.h"

#define MAX_LABEL_LENGTH 25
#define M_2_X_PI 6.28318530718
#define VISIBLE_ANGLE 72.0

void text_obj_free(text_obj *obj) {
    if (obj) {
        if (obj->glyphs_objs) {
            for (int g = 0; g < obj->n_glyphs; g++) {
                glyph_obj_free(obj->glyphs_objs[g]);
            }
            free(obj->glyphs_objs);
	    obj->glyphs_objs = NULL;
        }

        if (obj->glyphs_objs_2nd_line) {
            for (int g = 0; g < obj->n_glyphs_2nd_line; g++) {
                glyph_obj_free(obj->glyphs_objs_2nd_line[g]);
            }
            free(obj->glyphs_objs_2nd_line);
            obj->glyphs_objs_2nd_line = NULL;
        }

        free(obj);

    }
}

void text_obj_update_cnt_rad(text_obj *obj, SDL_Point center, int radius, int line, int n_lines) {
    if (obj) {

        if (obj->n_glyphs_2nd_line > 0) {
            radius = radius + obj->height * 0.5;
        }

        radius = radius + 0.8 * obj->height * (line + 0.5 * (n_lines-1));

        if (obj->glyphs_objs) {
            for (int g = 0; g < obj->n_glyphs; g++) {
                glyph_obj_update_cnt_rad(obj->glyphs_objs[g], center, radius);
            }
        }


        if (obj->glyphs_objs_2nd_line) {
            radius = radius - obj->height;
            for (int g = 0; g < obj->n_glyphs_2nd_line; g++) {
                glyph_obj_update_cnt_rad(obj->glyphs_objs_2nd_line[g], center, radius);
            }
        }
    }
}


text_obj *text_obj_new(SDL_Renderer *renderer, char *txt, char *icon, TTF_Font *font, TTF_Font *font_2nd_line, SDL_Color fg, SDL_Point center, int radius, int line, int n_lines, int light_x, int light_y) {

    if ((txt && strlen(txt) > 0) || icon) {
        Uint32 unicode_length = MAX_LABEL_LENGTH;
        Uint16 *unicode_text_2nd_line = NULL;
        Uint32 unicode_length_second_line = MAX_LABEL_LENGTH;
        Uint16 *unicode_text = NULL;
	if (txt) {
        	unicode_text = to_unicode(txt,&unicode_length,&unicode_text_2nd_line,&unicode_length_second_line);
	}

 	if (icon) {
		// Throw away a possible second line text, take the first line text as second line and the icon surface
		// as first line "text"
		if (unicode_text_2nd_line) {
			free(unicode_text_2nd_line);
		}
		if (unicode_text) {
			unicode_text_2nd_line = unicode_text;
			unicode_length_second_line = unicode_length;
			unicode_text = NULL;
		} else {
			unicode_text_2nd_line = NULL;
			unicode_length_second_line = 0;
		}
		unicode_length = 1;
	}


        text_obj *t = malloc(sizeof(text_obj));
        t->n_glyphs = unicode_length;
        t->glyphs_objs = malloc(unicode_length * sizeof(glyph_obj *));
        log_debug(MENU_CTX, "Rendering surface for %s\n",unicode_text);
        SDL_Surface *text_surface = icon ? IMG_Load(icon) : TTF_RenderUNICODE_Blended(font,unicode_text,fg);

        if (text_surface == NULL) {
            log_error(MENU_CTX, "Could not create glyph surface for \"[%s]\": %s\n", txt, icon ? IMG_GetError() : TTF_GetError());
            return NULL;
        }

        t->width = text_surface->w;
        t->height = text_surface->h;

        if (unicode_length_second_line > 0 && unicode_text_2nd_line != NULL) {
            radius = radius + text_surface->h * 0.3;
        }

        radius = radius + 0.8 * text_surface->h * (line + 0.5 * (n_lines-1));

	if (!icon) {
	        log_debug(MENU_CTX,"SDL_FreeSurface(text_surface => %p) (first line);\n", text_surface);
        	SDL_FreeSurface(text_surface);
	}

        unsigned int i = 0;
        for (i = 0; i < unicode_length; i++) {
            glyph_obj *glyph_o = icon ? glyph_obj_new_surface(renderer,text_surface,font,fg,center,radius) : glyph_obj_new(renderer,unicode_text[i],font,fg,center,radius);
            if (!glyph_o) {
                log_error(MENU_CTX, "Could not create glyph object for %c\n", unicode_text[i]);
                return NULL;
            }
            t->glyphs_objs[i] = glyph_o;
        }

        t->glyphs_objs_2nd_line = NULL;
        t->n_glyphs_2nd_line = unicode_length_second_line;
        if (unicode_length_second_line > 0 && unicode_text_2nd_line != NULL) {
            t->n_glyphs_2nd_line = unicode_length_second_line;
            t->glyphs_objs_2nd_line = malloc(unicode_length_second_line * sizeof(glyph_obj *));

            text_surface = TTF_RenderUNICODE_Blended(font_2nd_line,unicode_text_2nd_line,fg);
            if (text_surface == NULL) {
                log_error(MENU_CTX, "Could not create glyph surface for %s (unicode_text_2nd_line = %s, unicode_length_2nd_line = %d): %s\n", txt, unicode_text_2nd_line, unicode_length_second_line, TTF_GetError());
                return NULL;
            }

            t->width_2nd_line = text_surface->w;
            t->height_2nd_line = text_surface->h;
	    log_debug(MENU_CTX,"SDL_FreeSurface(text_surface => %p) (second line);\n", text_surface);
            SDL_FreeSurface(text_surface);

            unsigned int i = 0;
            int radius_new = radius - (0.6) * t->height - 1;
            for (i = 0; i < unicode_length_second_line; i++) {
                t->glyphs_objs_2nd_line[i] = glyph_obj_new(renderer,unicode_text_2nd_line[i],font_2nd_line,fg,center,radius_new);
            }
        }

	if (unicode_text) {
        	free(unicode_text);
	}
        if (unicode_text_2nd_line) {
            free(unicode_text_2nd_line);
        }

        return t;

    } else {
	    log_warning(MENU_CTX, "No icon and not label provided\n");
    }

    return NULL;

}

void text_obj_draw(SDL_Renderer *renderer, SDL_Texture *target, text_obj *label, int radius, int center_x, int center_y, double angle, double light_x, double light_y, int font_bumpmap, int shadow_offset, int shadow_alpha) {
    double circumference = M_2_X_PI * radius;

    log_config(MENU_CTX,"text_obj_draw: angle: %f, radius: %d, circumference: %f\n", angle, radius, circumference);
    if (radius == 0) {
        log_config(MENU_CTX,"text_obj_draw: radius = 0, returning\n");
        return;
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer,target);
    }

    double advance = 0;

    if (shadow_offset > 0) {
        advance = - 0.5 * label->width;
        for (int c = 0; c < label->n_glyphs; c++) {

            glyph_obj *glyph_obj = label->glyphs_objs[c];

            double crc = M_2_X_PI * glyph_obj->radius;
            double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/crc;
                
	    if (a != glyph_obj->current_angle) {
                    glyph_obj_update_bumpmap_texture(renderer, glyph_obj,center_x,center_y,a, light_x, light_y);
            }

            SDL_Texture *texture = font_bumpmap ? glyph_obj->bumpmap_overlay : glyph_obj->texture;

            if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {

                Uint8 orig_a, orig_r, orig_g, orig_b;
                SDL_GetTextureAlphaMod(texture, &orig_a);
                SDL_GetTextureColorMod(texture, &orig_r, &orig_g, &orig_b);
                SDL_SetTextureColorMod(texture,0,0,0);

                SDL_Rect shadow_dst_rec;
                shadow_dst_rec.w = glyph_obj->dst_rect->w;
                shadow_dst_rec.h = glyph_obj->dst_rect->h;

                for (int so = shadow_offset; so > 0; so--) {
                    int sa = (shadow_offset - so + 1)*shadow_alpha / (shadow_offset);
                    SDL_SetTextureAlphaMod(texture,sa);
                    shadow_dst_rec.x = glyph_obj->dst_rect->x+so*glyph_obj->shadow_dx;
                    shadow_dst_rec.y = glyph_obj->dst_rect->y+so*glyph_obj->shadow_dy;
                    SDL_RenderCopyEx(renderer,texture,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);
                }

                SDL_SetTextureAlphaMod(texture,orig_a);
                SDL_SetTextureColorMod(texture,orig_r, orig_g, orig_b);
            }

            advance += glyph_obj->advance;

        }

        advance = - 0.5 * label->width_2nd_line;
        for (int c = 0; c < label->n_glyphs_2nd_line; c++) {

            glyph_obj *glyph_obj = label->glyphs_objs_2nd_line[c];

            double crc = M_2_X_PI * glyph_obj->radius;
            double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/crc;
	    if (a != glyph_obj->current_angle) {
                    glyph_obj_update_bumpmap_texture(renderer, glyph_obj,center_x,center_y,a, light_x, light_y);
            }
            SDL_Texture *texture = font_bumpmap ? glyph_obj->bumpmap_overlay : glyph_obj->texture;

            if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {
                Uint8 orig_a, orig_r, orig_g, orig_b;
                SDL_GetTextureAlphaMod(texture, &orig_a);
                SDL_GetTextureColorMod(texture, &orig_r, &orig_g, &orig_b);
                SDL_SetTextureColorMod(texture,0,0,0);

                SDL_Rect shadow_dst_rec;
                shadow_dst_rec.w = glyph_obj->dst_rect->w;
                shadow_dst_rec.h = glyph_obj->dst_rect->h;

                for (int so = shadow_offset; so > 0; so--) {
                    int sa = (shadow_offset - so + 1)*shadow_alpha / (shadow_offset);
                    SDL_SetTextureAlphaMod(texture,sa);
                    shadow_dst_rec.x = glyph_obj->dst_rect->x+so*glyph_obj->shadow_dx;
                    shadow_dst_rec.y = glyph_obj->dst_rect->y+so*glyph_obj->shadow_dy;
                    SDL_RenderCopyEx(renderer,texture,NULL,&shadow_dst_rec,a, glyph_obj->rot_center,SDL_FLIP_NONE);
                }

                SDL_SetTextureAlphaMod(texture,orig_a);
                SDL_SetTextureColorMod(texture,orig_r, orig_g, orig_b);

            }

            advance += glyph_obj->advance;

        }
    } else {
        log_error(MENU_CTX, "No shadow\n");
    }

    advance = - 0.5 * label->width;
    for (int c = 0; c < label->n_glyphs; c++) {
        glyph_obj *glyph_obj = label->glyphs_objs[c];

        double crc = M_2_X_PI * glyph_obj->radius;
        double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/crc;

        if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {

            if (font_bumpmap) {

                if (a != glyph_obj->current_angle) {
                    glyph_obj_update_bumpmap_texture(renderer, glyph_obj,center_x,center_y,a, light_x, light_y);
                }

                SDL_Texture *texture = glyph_obj->bumpmap_overlay;

                SDL_RenderCopyEx(renderer,texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            } else {

                SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            }
        } else {
            log_debug(MENU_CTX, "angle %f not in visible range of %f\n", angle, VISIBLE_ANGLE);
        }

        glyph_obj->current_angle = a;
        advance += glyph_obj->advance;

    }

    advance = - 0.5 * label->width_2nd_line;
    for (int c = 0; c < label->n_glyphs_2nd_line; c++) {
        glyph_obj *glyph_obj = label->glyphs_objs_2nd_line[c];

        double crc = M_2_X_PI * glyph_obj->radius;
        double a = angle + 360.0 * (advance + 0.5 * glyph_obj->dst_rect->w)/crc;

        if (a >= -VISIBLE_ANGLE && a <= VISIBLE_ANGLE) {

            if (font_bumpmap) {

                if (a != glyph_obj->current_angle) {
                    glyph_obj_update_bumpmap_texture(renderer, glyph_obj,center_x,center_y,a, light_x, light_y);
                }

                SDL_Texture *texture = glyph_obj->bumpmap_overlay;
                log_trace(MENU_CTX,"texture: %p\n", texture);

                SDL_RenderCopyEx(renderer,texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            } else {
		    
                SDL_RenderCopyEx(renderer,glyph_obj->texture,NULL,glyph_obj->dst_rect,a, glyph_obj->rot_center,SDL_FLIP_NONE);

            }
        } else {
            log_debug(MENU_CTX, "angle %f not in visible range of %f\n", angle, VISIBLE_ANGLE);
        }

        glyph_obj->current_angle = a;
        advance += glyph_obj->advance;
    }

    if (target != NULL) {
        SDL_SetRenderTarget(renderer,NULL);
        SDL_RenderCopy(renderer,target,NULL,NULL);
    }

}
