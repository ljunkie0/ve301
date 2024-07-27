/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>
#include <mpd/client.h>
#include "base.h"
#include "menu.h"
#include "audio.h"
#include "weather.h"
#include "bluetooth.h"
#include "sdl_util.h"

#ifdef RASPBERRY
#ifdef SDL1
#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#else
#define DEFAULT_WINDOW_WIDTH 720
#define WINDOW_HEIGHT 576
#endif
#else
#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#endif
#define NO_OF_SCALES 60

#ifdef RASPBERRY
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#else
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#endif
#define DEFAULT_X_OFFSET 0
#define DEFAULT_Y_OFFSET 0
#define DEFAULT_LABEL_RADIUS 100
#define DEFAULT_SCALES_RADIUS_START 120
#define DEFAULT_SCALES_RADIUS_END 200
#define DEFAULT_ANGLE_OFFSET 0.0
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_INFO_FONT_SIZE 24
#define CALLBACK_SECONDS 5
#define CHECK_INTERNET_SECONDS 60
#define CHECK_BLUETOOTH_SECONDS 1
#define INFO_MENU_SECONDS 14
#define INFO_MENU_ITEM_SECONDS 5

#define NIGHT_HUE 0.657
#define NIGHT_SAT 0.466
#define NIGHT_VAL 0.161
#define DAY_HUE   0.165
#define DAY_SAT   0.227
#define DAY_VAL   0.971

#ifdef PERF_TEST
/**
 * Performance test
 */
#define MENU_ACTION_LISTENER menu_action_listener_dbg
#define MENU_CALL_BACK menu_call_back_dbg
#else
#define MENU_ACTION_LISTENER menu_action_listener
#define MENU_CALL_BACK menu_call_back
#endif

typedef enum {
    ACTIVE, STANDBY
} state;

typedef enum {
    OBJ_TYPE_SONG, OBJ_TYPE_PLAYLIST, OBJ_TYPE_DIRECTORY, OBJ_TYPE_TIME
} object_type;

static const char *time_menu_item_format = "%H:%M";
static const char *bluetooth_label = "Bluetooth";
static int info_menu_item_seconds = INFO_MENU_ITEM_SECONDS;

#ifndef M_PI
#define M_PI 3.1415926536
#endif

typedef struct station {
    const char *label;
    const char *url;
    int id;
} station;

static int last_tm_min;
static struct tm* current_tm_info;
static int hsv_style = 0;

static time_t info_menu_t = 0;
static time_t callback_t = 0;
static time_t check_internet_t = 0;
static time_t check_bluetooth_t = 0;

static int internet_available;

static menu *radio_menu;
static menu *info_menu;
static menu *nav_menu;
static menu *lib_menu;
static menu *root_dir_menu;
static menu *album_menu;
static menu *song_menu;
static menu *current_menu;
static menu_item *time_item;
static menu_item *title_item;
static menu_item *name_item;
static menu_item *weather_item;
static menu_item *temperature_item;
static menu *volume_menu;
static menu_item *volume_menu_item;
static menu *settings_menu;

static weather wthr;

static int bt_device_status = 0;

static menu_ctrl *ctrl;

static playlist *album_songs = NULL;

static song *current_song;

static theme *default_theme = NULL;
static theme *bluetooth_theme = NULL;
static theme *spotify_theme = NULL;

const char *get_vol_label(int volume) {
    char *vol_txt = malloc(13*sizeof(char));
    sprintf(vol_txt,"Volume: %d%%",volume);
    return vol_txt;
}

int item_action_update_directories_menu(menu_event evt, menu *m, menu_item *item);

station *new_station(const char *label, const char *url) {
    station *s = malloc(sizeof(station));
    s->label = label;
    s->url = url;
    s->id = add_station(url);
    log_info(MAIN_CTX, "Adding station %s with id %d\n", s->label, s->id);
    return s;
}

void reset_info_menu_timer() {
    time_t timer;
    time(&timer);
    info_menu_t = timer;
    callback_t = timer;
}

void update_radio_menu() {
    menu_clear(radio_menu);
    playlist *internet_radios = get_internet_radios();
    if (internet_radios != NULL) {
            unsigned int r = 0;
            for (r = 0; r < internet_radios->n_songs; r++) {
                    song *s = internet_radios->songs[r];
                    menu_item_new(radio_menu, s->title, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
            }
    } else {
            log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
            menu_item_new(radio_menu, "FEHLER", NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_album_menu() {
    menu_clear(album_menu);
    unsigned int a;
    char **albums = get_albums(&a);
    if (albums != NULL) {
        log_info(MAIN_CTX, "Album successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_info(MAIN_CTX, "Getting album %d\n", r);
            menu_add_sub_menu(album_menu, albums[r], song_menu, NULL);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
        menu_item_new(album_menu, "FEHLER", NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_song_menu(menu_item *album_item) {
    log_info(MAIN_CTX, "Update song menu: %s\n", album_item->unicode_label);
    int id = 0;
    for (id = 0; id <= song_menu->max_id; id++) {
        dispose_song((song *)song_menu->item[id]->object);
    }
    menu_clear(song_menu);
    if (album_songs) {
        dispose_playlist(album_songs);
    }
    album_songs = get_album_songs(album_item->utf8_label);
    if (album_songs != NULL) {
        unsigned int r = 0;
        for (r = 0; r < album_songs->n_songs; r++) {
            song *s = album_songs->songs[r];
            log_info(MAIN_CTX, "Song: %s\n", s->title);
            menu_item_new((menu *) album_item->sub_menu, s->title, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
        menu_item_new((menu *)album_item->sub_menu, "FEHLER", NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
    }
}

void update_directory_menu(menu_item *directory_item) {
    menu *m = (menu *) directory_item->menu;
    menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
    if (directory_item->sub_menu) {
        menu_clear((menu *)directory_item->sub_menu);
    } else {
        directory_item->sub_menu = menu_new((menu_ctrl *)m->ctrl, m->n_o_lines);
    }
    menu *sub_menu = (menu *) directory_item->sub_menu;
    struct mpd_connection *mpd_conn = get_mpd_connection();
    if (mpd_conn) {
        if (directory_item->object_type != OBJ_TYPE_DIRECTORY) {
            log_error(MAIN_CTX, "Item is not of type directory\n");
            return;
        }
        const char *dir = (const char *) directory_item->object;
        if (!mpd_send_list_meta(mpd_conn,dir)) {
            log_error(MAIN_CTX, "Could not list directory %s\n", dir);
            return;
        }
        struct mpd_entity *entity = mpd_recv_entity(mpd_conn);
        while (entity) {
            log_info(MAIN_CTX, "Type: %d\n", mpd_entity_get_type(entity));
            enum mpd_entity_type type = mpd_entity_get_type(entity);
            if (type == MPD_ENTITY_TYPE_DIRECTORY) {
                const struct mpd_directory *sub_dir = mpd_entity_get_directory(entity);
                const char *path = mpd_directory_get_path(sub_dir);
                char *name = get_name_from_path(path);
                log_info(MAIN_CTX, "Directory %s\n", name);
                menu *sub_sub_menu = menu_new(ctrl,m->n_o_lines);
                menu_item *sub_sub_menu_item = menu_add_sub_menu(sub_menu,name,sub_sub_menu, item_action_update_directories_menu);
                sub_sub_menu_item->object = (const void *) path;
                sub_sub_menu_item->object_type = OBJ_TYPE_DIRECTORY;
            } else if (type == MPD_ENTITY_TYPE_SONG) {
                const struct mpd_song *s = mpd_entity_get_song(entity);
                const char *uri = mpd_song_get_uri(s);
                char *name = get_name_from_path(uri);
                log_info(MAIN_CTX, "Song: %s\n", name);
                song *my_song = malloc(sizeof(song));
                my_song->name = name;
                my_song->url = uri;
                my_song->id = mpd_song_get_id(s);
                my_song->title = (char *) mpd_song_get_tag(s,MPD_TAG_TITLE,0);
                menu_item_new(sub_menu,my_song->title,my_song,OBJ_TYPE_SONG,NULL,-1, NULL, NULL, -1);
            }
            entity = mpd_recv_entity(mpd_conn);
        }
    } else {

    }
}

void update_time_item(menu_item *time_menu_item) {
    last_tm_min = current_tm_info->tm_min;
    char *buffer = malloc(25 * sizeof(char));
    strftime(buffer, 25, time_menu_item_format, current_tm_info);
    menu_item_update_label(time_menu_item, buffer);
    log_debug(MAIN_CTX, "free(buffer))\n");
    log_debug(MAIN_CTX, "buffer = %p\n", buffer);
    free(buffer);
}

int change_x_offset(menu_ctrl *ctrl, int c) {
    menu_ctrl_set_offset(ctrl,ctrl->x_offset + c,ctrl->y_offset);
    set_config_value_int("x_offset",ctrl->x_offset);
    menu_ctrl_draw(ctrl);
    return 0;
}

int change_y_offset(menu_ctrl *ctrl, int c) {
    menu_ctrl_set_offset(ctrl,ctrl->x_offset,ctrl->y_offset + c);
    set_config_value_int("y_offset",ctrl->y_offset);
    menu_ctrl_draw(ctrl);
    return 0;
}

int item_action_update_directories_menu(menu_event evt, menu *m, menu_item *item) {
    update_directory_menu(item);
    return 0;
}

int menu_action_listener_dbg(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    return 0;
}

int menu_action_listener(menu_event evt, menu *m_ptr, menu_item *item_ptr) {
    log_config(MAIN_CTX, "action(%d)\n",evt);
    if (evt == ACTIVATE) {
        menu_item *item = (menu_item *) item_ptr;
        log_debug(MAIN_CTX, "Action: %s\n", item->unicode_label);
        if (item->is_sub_menu) {
            log_info(MAIN_CTX, "Sub menu: %s\n", item->unicode_label);
            if (item->sub_menu == radio_menu) {
                update_radio_menu();
            } else if (item->sub_menu == album_menu) {
                update_album_menu();
            } else if (item->sub_menu == song_menu) {
                update_song_menu(item);
            } else if (item->object_type == OBJ_TYPE_DIRECTORY || item->sub_menu == root_dir_menu) {
                update_directory_menu(item);
            }
        } else if (item->object && item->object_type == OBJ_TYPE_SONG) {
            song *s = (song *) item->object;
            song *p = get_playing_song();
            if (!p || (p->id != s->id)) {
                if (!play_song(s)) {
                    log_error(MAIN_CTX, "Could not play song %s", s->title);
                }
                song *current_song = get_playing_song();
                if (current_song) {
                    menu_item_update_label(name_item, current_song->name);
                }
            } else if (p) {
                stop();
            }
        } else if (((menu *)item->menu)->transient) {
            if (ctrl->active) {
                menu_open(ctrl->active);
            } else {
                menu_open(nav_menu);
            }
        }
    } else if (evt == ACTIVATE_1) {
        //stand_by();
    } else if (evt == TURN_LEFT_1) {
            menu_item_show(volume_menu_item);
            decrease_volume();
            const char *vol_label = get_vol_label(get_volume());
            menu_item_update_label(volume_menu_item,vol_label);
            menu_item_show(volume_menu_item);
    } else if (evt == TURN_RIGHT_1) {
            menu_item_show(volume_menu_item);
            increase_volume();
            const char *vol_label = get_vol_label(get_volume());
            menu_item_update_label(volume_menu_item,vol_label);
            menu_item_show(volume_menu_item);
    } else if (evt == DISPOSE) {
        menu_item *item = (menu_item *) item_ptr;
        const void *obj = item->object;
        if (obj && item->menu != radio_menu) {
            if (item->object_type == OBJ_TYPE_DIRECTORY || item->object_type == OBJ_TYPE_SONG) {
                log_debug(MAIN_CTX, "free(%p)\n",obj);
                free((void *)obj);
            } else if (item->object_type == OBJ_TYPE_SONG) {
                dispose_song((song *)obj);
            }
        }
    }

    reset_info_menu_timer();

    return 0;
}

int change_label_radius(menu_ctrl *ctrl, int c) {
    int label_radius = ctrl->root[0]->radius_labels + c;
    set_config_value_int("radius_labels",label_radius);
    menu_ctrl_set_radii(ctrl, label_radius, -1, -1);
    menu_ctrl_draw(ctrl);
    return 0;
}

int change_scale_radius(menu_ctrl *ctrl, int c) {
    int scale_radius = ctrl->radius_scales_start + c;
    set_config_value_int("radius_scales_start",scale_radius);
    menu_ctrl_set_radii(ctrl, scale_radius, -1, -1);
    menu_ctrl_draw(ctrl);
    return 0;
}

int item_action_x_offset(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        change_x_offset(m->ctrl,-1);
        menu_item_show(item);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        change_x_offset(m->ctrl,1);
        menu_item_show(item);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_y_offset(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        change_y_offset(m->ctrl,1);
        menu_item_show(item);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        change_y_offset(m->ctrl,-1);
        menu_item_show(item);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_label_radius(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        change_label_radius(m->ctrl,1);
        menu_item_show(item);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        change_label_radius(m->ctrl,-1);
        menu_item_show(item);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_background_hue(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->background_color->r,m->ctrl->background_color->g,m->ctrl->background_color->b,&h,&s,&v);
        h = h + 2;
        menu_ctrl_set_bg_color_hsv(m->ctrl,h,s,v);
        Uint8 r,g,b;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        char *html = rgb_to_html(r,g,b);
        set_config_value("background_color",html);
        free(html);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->background_color->r,m->ctrl->background_color->g,m->ctrl->background_color->b,&h,&s,&v);
        h = h - 2;
        menu_ctrl_set_bg_color_hsv(m->ctrl,h,s,v);
        Uint8 r,g,b;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        char *html = rgb_to_html(r,g,b);
        set_config_value("background_color",html);
        free(html);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_default_hue(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->default_color->r,m->ctrl->default_color->g,m->ctrl->default_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h + 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_default_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->default_color->r,m->ctrl->default_color->g,m->ctrl->default_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h - 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_default_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_selected_hue(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->selected_color->r,m->ctrl->selected_color->g,m->ctrl->selected_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h + 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_selected_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->selected_color->r,m->ctrl->selected_color->g,m->ctrl->selected_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h - 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_selected_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_active_hue(menu_event evt, menu *m, menu_item *item) {
    if (evt == TURN_LEFT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->activated_color->r,m->ctrl->activated_color->g,m->ctrl->activated_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h + 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_active_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    } else if (evt == TURN_RIGHT_1) {
        double h,s,v;
        rgb_to_hsv(m->ctrl->activated_color->r,m->ctrl->activated_color->g,m->ctrl->activated_color->b,&h,&s,&v);
        Uint8 r,g,b;
        h = h - 5;
        hsv_to_rgb(h,s,v,&r,&g,&b);
        menu_ctrl_set_active_color_rgb(m->ctrl,r,g,b);
        menu_ctrl_draw(ctrl);
        reset_info_menu_timer();
    }
    return 0;
}

int item_action_store_config(menu_event evt, menu *m, menu_item *item) {
    write_config();
    return 0;
}

int item_action_scale_radius_decrease(menu_event evt, menu *m, menu_item *item) {
    change_scale_radius(m->ctrl,-10);
    return 0;
}

theme *get_config_theme(char *theme_name) {
    theme *th = malloc (sizeof(theme));
    th->background_color = get_config_value_group("background_color", get_config_value("background_color", "#ffffff"), theme_name);
    th->bg_color_of_time = get_config_value_int_group("bg_color_from_time", get_config_value_int("bg_color_from_time", 0), theme_name);
    th->scale_color = get_config_value_group("scale_color", get_config_value("scale_color", "#ff0000"), theme_name);
    th->indicator_color = get_config_value_group("indicator_color", get_config_value("indicator_color", "#ff0000"), theme_name);
    th->default_color = get_config_value_group("default_color", get_config_value("default_color", "#00ff00"), theme_name);
    th->selected_color = get_config_value_group("selected_color", get_config_value("selected_color", "#0000ff"), theme_name);
    th->activated_color = get_config_value_group("activated_color", get_config_value("activated_color", "#00ffff"), theme_name);
    th->bg_image_path = get_config_value_group("bg_image_path", get_config_value("bg_image_path", 0), theme_name);
    th->font_bumpmap = get_config_value_int_group("font_bumpmap", get_config_value_int("font_bumpmap", 0), theme_name);
    th->shadow_offset = get_config_value_int_group("shadow_offset",get_config_value_int("shadow_offset",0), theme_name);
    th->shadow_alpha = get_config_value_int_group("shadow_alpha", get_config_value_int("shadow_alpha", 0), theme_name);
    th->bg_color_palette = NULL;
    th->bg_cp_colors = 0;
    th->fg_color_palette = NULL;
    th->fg_cp_colors = 0;

    char *bg_color_palette = get_config_value("bg_color_palette", 0);
    if (bg_color_palette) {
        th->bg_cp_colors = 0;
        char *color = strtok(bg_color_palette,",");
        while (color != NULL) {
            th->bg_cp_colors = th->bg_cp_colors + 1;
            th->bg_color_palette = realloc(th->bg_color_palette,th->bg_cp_colors*sizeof(char *));
            th->bg_color_palette[th->bg_cp_colors-1] = color;
            color = strtok(NULL,",");
        }
    }

    char *fg_color_palette = get_config_value("fg_color_palette", 0);
    if (fg_color_palette) {
        th->fg_cp_colors = 0;
        char *color = strtok(fg_color_palette,",");
        while (color != NULL) {
            th->fg_cp_colors = th->fg_cp_colors + 1;
            th->fg_color_palette = realloc(th->fg_color_palette,th->fg_cp_colors*sizeof(char *));
            th->fg_color_palette[th->fg_cp_colors-1] = color;
            color = strtok(NULL,",");
        }
    }

    return th;
}

void free_theme(theme *th) {
    log_debug(MAIN_CTX, "free_theme(%p)\n",th);
    if (th->background_color)
        free(th->background_color);
    if (th->scale_color)
        free(th->scale_color);
    if (th->indicator_color)
        free(th->indicator_color);
    if (th->default_color)
        free(th->default_color);
    if (th->selected_color)
        free(th->selected_color);
    if (th->activated_color)
        free(th->activated_color);
    if (th->bg_image_path)
        free(th->bg_image_path);
    if (th->bg_color_palette)
        free(th->bg_color_palette);
    if (th->fg_color_palette)
        free(th->fg_color_palette);
    free(th);
}

static time_t info_menu_t_dbg = -1;

static long perf_timer_ms = 0;
// #define SECS_IN_DAY (24 * 60 * 60)

int menu_call_back_dbg(menu_ctrl *m_ptr) {

    menu_ctrl *ctrl = (menu_ctrl *) m_ptr;

    if (ctrl->current != radio_menu) {
        menu_open(radio_menu);
    }

    if (ctrl && ctrl->current) {
        int current_id = ctrl->current->current_id+1;
        if (current_id > ctrl->current->max_id) {
            current_id = 0;
        }

        struct timespec perf_timer_s, perf_timer_e;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &perf_timer_s);
        long msecs_s = perf_timer_s.tv_sec * 1000 + perf_timer_s.tv_nsec / 1000000;
        menu_item_warp_to(ctrl->current->item[current_id]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &perf_timer_e);
        long msecs_e = perf_timer_e.tv_sec * 1000 + perf_timer_e.tv_nsec / 1000000;

        long msecs_p = msecs_e - msecs_s;
        perf_timer_ms += msecs_p;

/*        printf("%-15s: %10jd.%03ld (", "START",
               (intmax_t) perf_timer_s.tv_sec, perf_timer_s.tv_nsec / 1000000);
        long days = perf_timer_s.tv_sec / SECS_IN_DAY;
        if (days > 0)
            printf("%ld days + ", days);
        printf("%2dh %2dm %2ds",
               (int) (perf_timer_s.tv_sec % SECS_IN_DAY) / 3600,
               (int) (perf_timer_s.tv_sec % 3600) / 60,
               (int) perf_timer_s.tv_sec % 60);
        printf(")\n");

        printf("%-15s: %10jd.%03ld (", "END",
               (intmax_t) perf_timer_e.tv_sec, perf_timer_e.tv_nsec / 1000000);
        days = perf_timer_e.tv_sec / SECS_IN_DAY;
        if (days > 0)
            printf("%ld days + ", days);
        printf("%2dh %2dm %2ds",
               (int) (perf_timer_e.tv_sec % SECS_IN_DAY) / 3600,
               (int) (perf_timer_e.tv_sec % 3600) / 60,
               (int) perf_timer_e.tv_sec % 60);
        printf(")\n");
        perf_timer_ns += 1000000 * (perf_timer_e.tv_sec - perf_timer_s.tv_sec) + perf_timer_e.tv_nsec - perf_timer_s.tv_nsec;
*/

        time_t timer;
        time(&timer);
        current_tm_info = localtime(&timer);

        if (info_menu_t_dbg < 0) {
            info_menu_t_dbg = timer;
        }

        long time_diff = timer - info_menu_t_dbg;

        if (time_diff > 30) {
            printf("Time: %ld\n", perf_timer_ms);
            menu_ctrl_quit(ctrl);
            base_close();
            exit(0);
        }
    }

    return 1;
}

int menu_call_back(menu_ctrl *m_ptr) {

    log_debug(MAIN_CTX, "Start: Callback\n");

    menu_ctrl *ctrl = (menu_ctrl *) m_ptr;

    time_t timer;
    time(&timer);
    current_tm_info = localtime(&timer);

    long time_diff = timer - info_menu_t;
    long callback_diff = timer - callback_t;
    long check_internet_diff = timer - check_internet_t;
    long check_bluetooth_diff = timer - check_bluetooth_t;

    if (ctrl->current != info_menu) {
        current_menu = ctrl->current;
    }

    log_debug(MAIN_CTX, "Callback diff: %d\n", callback_diff);

    if (check_internet_diff > CHECK_INTERNET_SECONDS) {

        check_internet_t = timer;
        internet_available = check_internet();

    }

    if (check_bluetooth_diff > CHECK_BLUETOOTH_SECONDS) {
        if (bt_connection_signal()) {
            if (bt_is_connected()) {
                if (!bt_device_status) {
                    bt_device_status = 1;
                    stop();
                    if (bluetooth_theme) {
                        menu_ctrl_apply_theme(ctrl, bluetooth_theme);
                    }
                    menu_item_update_label(title_item, "Bluetooth");
                    menu_item_update_label(name_item, "Bluetooth");
                }

                char *title = bt_get_title();
                if (title) {
                    menu_item_update_label(title_item, title);
                } else {
                    menu_item_update_label(title_item, bluetooth_label);
                }

                char *artist = bt_get_artist();
                if (artist) {
                    menu_item_update_label(name_item, artist);
                } else {
                    menu_item_update_label(name_item, bluetooth_label);
                }

            } else {
                if (bt_device_status) {
                    menu_ctrl_apply_theme(ctrl, default_theme);
                    menu_item_update_label(title_item, "VE301");
                    menu_item_update_label(name_item, "VE301");
                    bt_device_status = 0;
                }
            }
        }
    }

    if (callback_diff > CALLBACK_SECONDS) {

        callback_t = timer;

        if (hsv_style) {

            double day_seconds = (current_tm_info->tm_sec + current_tm_info->tm_min * 60.0 + current_tm_info->tm_hour * 3600.0);
            log_info(MAIN_CTX, "Day seconds: %f\n", day_seconds);
            double day_fraction = day_seconds / 86400.0;
            log_info(MAIN_CTX, "Day fraction: %f\n", day_fraction);
            double color_temp = 1000.0 + day_fraction * (11100.0 - 1000.0);
            log_info(MAIN_CTX, "Color temp: %f\n", color_temp);

            Uint8 r,g,b;

            double value = 0.5 * (1.0-cos(2.0*day_fraction*M_PI));

            color_temp_to_rgb(color_temp,&r,&g,&b,value);

            menu_ctrl_set_bg_color_rgb(ctrl, r,g,b);

            if (value > 0.2) {
                menu_ctrl_set_active_color_hsv(ctrl, 0, 0, 0.0);
                menu_ctrl_set_default_color_hsv(ctrl, 0, 0, 0.0);
                menu_ctrl_set_selected_color_hsv(ctrl, 0, 0, 0.0);
            } else {
                menu_ctrl_set_active_color_hsv(ctrl, 0, 0, 1.0);
                menu_ctrl_set_default_color_hsv(ctrl, 0, 0, 1.0);
                menu_ctrl_set_selected_color_hsv(ctrl, 0, 0, 1.0);
            }

        }

        if (!bt_device_status) {
            current_song = get_playing_song();
            if (current_song) {
                log_debug(MAIN_CTX, "Current song: %s\n", current_song->title);
                if (current_song->title && (strlen(current_song->title) > 0) && strncmp(current_song->title, "N/A", 3)) {
                    menu_item_update_label(title_item, current_song->title);
                }
                if (current_song->name && (strlen(current_song->name) > 0) && strncmp(current_song->name, "N/A", 3)) {
                    menu_item_update_label(name_item, current_song->name);
                }
            }
        }

        if (weather_item && temperature_item) {
            char *temp_str = malloc(10*sizeof(char));
            sprintf(temp_str,"%.1f°C",((double)round(10.0*wthr.temp))/10);
            menu_item_update_label(temperature_item,temp_str);

            if (wthr.weather_icon) {
                menu_item_update_label(weather_item,wthr.weather_icon);
            }
        }

        update_time_item(time_item);

    }

    if (time_diff > info_menu_item_seconds) {
        info_menu_t = timer;
        if (internet_available) {
            int current_id = ctrl->root[0]->current_id+1;
            if (current_id > ctrl->root[0]->max_id) {
                current_id = 0;
            }
            menu_item_warp_to(ctrl->root[0]->item[current_id]);
        } else {
            menu_item_update_label(title_item,"No internet");
            menu_item_warp_to(title_item);
        }
    }

    log_debug(MAIN_CTX, "End:  Callback\n");

    return 1;

}

int weather_lstnr(weather *weather) {
    if (weather) {

        wthr.temp = weather->temp;
        wthr.weather_icon = my_copystr((const char *) weather->weather_icon);
        log_info(MAIN_CTX, "Received new weather: %.0f°C [%s]\n",weather->temp, weather->weather_icon);

    }

    return 1;

}

menu_ctrl *create_menu() {

    char *font = get_config_value("font", DEFAULT_FONT);
    int font_size = get_config_value_int("font_size",DEFAULT_FONT_SIZE);
    int info_font_size = get_config_value_int("info_font_size",DEFAULT_INFO_FONT_SIZE);
    char *weather_font = get_config_value("weather_font", DEFAULT_FONT);
    int weather_font_size = get_config_value_int("weather_font_size", info_font_size);
    int temp_font_size = get_config_value_int("temperature_font_size", info_font_size);
    int time_font_size = get_config_value_int("time_font_size",info_font_size);
    int w = get_config_value_int("window_width",DEFAULT_WINDOW_WIDTH);
    int y_offset = get_config_value_int("y_offset",DEFAULT_Y_OFFSET);
    int x_offset = get_config_value_int("x_offset",DEFAULT_Y_OFFSET);
    double angle_offset = get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
    int radius_labels = get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
    int radius_scales_start = get_config_value_int("radius_scales_start", DEFAULT_SCALES_RADIUS_START);
    int radius_scales_end = get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_END);
    int draw_scales = get_config_value_int("draw_scales",1);
    int light_x = get_config_value_int("light_x", (int) w/2);
    int light_y = get_config_value_int("light_y", 100);
    int light_radius = get_config_value_int("light_radius", 300);
    int light_alpha = get_config_value_int("light_alpha", 0);
    char *light_img = get_config_value("light_image_path",NULL);
    int radio_radius_labels = get_config_value_int("radio_radius_labels", radius_labels);
    info_menu_item_seconds = get_config_value_int("info_menu_item_seconds",INFO_MENU_ITEM_SECONDS);
    hsv_style = get_config_value_int("hsv_style",0);

    default_theme = get_config_theme ("Default");
    bluetooth_theme = get_config_theme("Bluetooth");
    spotify_theme = get_config_theme("Spotify");

    menu_ctrl *ctrl = menu_ctrl_new(w, x_offset, y_offset, radius_labels, draw_scales, radius_scales_start, radius_scales_end, angle_offset, font, font_size, 0, &MENU_ACTION_LISTENER, &MENU_CALL_BACK);

    if (ctrl) {

        menu_ctrl_set_light(ctrl,light_x,light_y,light_radius,light_alpha);
        if (light_img) {
            menu_ctrl_set_light_img(ctrl,light_img);
        }

        menu_ctrl_apply_theme (ctrl,default_theme);

        ctrl->root[0]->current_id = 0;

        time_t timer;
        time(&timer);
        struct tm *tm_info = localtime(&timer);
        last_tm_min = tm_info->tm_min;

        char *buffer = malloc(25 * sizeof(char));
        strftime(buffer, 25, time_menu_item_format, tm_info);

        time_item = menu_item_new(ctrl->root[0], buffer, NULL, OBJ_TYPE_TIME, font, time_font_size, NULL, NULL, -1);
        free(buffer);
        title_item = menu_item_new(ctrl->root[0], "VE 301", NULL, UNKNOWN_OBJECT_TYPE, font, info_font_size, NULL, NULL, -1);
        name_item = menu_item_new(ctrl->root[0], "VE 301", NULL, UNKNOWN_OBJECT_TYPE, font, info_font_size, NULL, NULL, -1);

        const char *weather_api_key = get_config_value("weather_api_key", "");
        const char *weather_location = get_config_value("weather_location", "");
        const char *weather_units = get_config_value("weather_units", "metric");
        if (strlen(weather_api_key) > 0 && strlen(weather_location) > 0) {
            init_weather(120,weather_api_key,weather_location,weather_units);
            weather_item = menu_item_new(ctrl->root[0], " ", NULL, UNKNOWN_OBJECT_TYPE, weather_font, weather_font_size, NULL, font, font_size);
            temperature_item = menu_item_new(ctrl->root[0], "Weather", NULL, UNKNOWN_OBJECT_TYPE, font, temp_font_size, NULL, font, font_size);
            start_weather_thread(&weather_lstnr);
        } else {
            weather_item = NULL;
        }

        info_menu = ctrl->root[0];
        info_menu->transient = 1;
        ctrl->active = NULL;

        radio_menu = menu_new(ctrl, 3);
        radio_menu->radius_labels = radio_radius_labels;
        radio_menu->n_o_items_on_scale = radio_menu->n_o_lines * ctrl->n_o_items_on_scale;
        playlist *internet_radios = get_internet_radios();
        if (internet_radios != NULL) {
            Uint32 r = 0;
            for (r = 0; r < internet_radios->n_songs; r++) {
                song *s = internet_radios->songs[r];
                menu_item_new(radio_menu, s->title, s, OBJ_TYPE_SONG, NULL, -1, NULL, NULL, -1);
            }
        } else {
            log_error(MAIN_CTX, "create_menu: playlist is NULL\n");
            menu_item_new(radio_menu, "FEHLER", NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        }

        lib_menu = menu_new(ctrl,1);
        album_menu = menu_new(ctrl,1);
        menu_add_sub_menu(lib_menu, "Alben", album_menu, NULL);
        nav_menu = menu_new_root(ctrl, 1);
        menu_add_sub_menu(nav_menu, "Radio", radio_menu, NULL);
        menu_add_sub_menu(nav_menu, "Bibliothek", lib_menu, NULL);

        settings_menu = menu_new(ctrl,1);
        menu_item_new(settings_menu, "X Offset", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_x_offset, NULL, -1);
        //menu_item_new(settings_menu, "X Offset-", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_offset_left, NULL, -1);
        menu_item_new(settings_menu, "Y Offset", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_y_offset, NULL, -1);
        menu_item_new(settings_menu, "Label Radius", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_label_radius, NULL, -1);
        menu_item_new(settings_menu, "Einstellungen Speichern", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_store_config, NULL, -1);
        menu_add_sub_menu(nav_menu, "Einstellungen", settings_menu, NULL);

        menu *themes_menu = menu_new(ctrl,1);
        menu_item_new(themes_menu, "Background", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_background_hue, NULL, -1);
        menu_item_new(themes_menu, "Default Foreground", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_default_hue, NULL, -1);
        menu_item_new(themes_menu, "Selected Foreground", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_selected_hue, NULL, -1);
        menu_item_new(themes_menu, "Active Foreground", NULL, OBJECT_TYPE_ACTION, NULL, -1, &item_action_active_hue, NULL, -1);

        menu_add_sub_menu(settings_menu,"Theme", themes_menu, NULL);

        root_dir_menu = menu_new(ctrl,1);
        menu_item *dir_menu_item = menu_add_sub_menu(nav_menu, "Verzeichnisse", root_dir_menu, item_action_update_directories_menu);
        dir_menu_item->object = "/";
        dir_menu_item->object_type = OBJ_TYPE_DIRECTORY;
        song_menu = menu_new(ctrl,1);

        volume_menu = menu_new_root(ctrl,1);
        volume_menu->transient = 1;
        volume_menu->draw_only_active = 1;
        int volume = get_volume();
        volume_menu_item = menu_item_new(volume_menu,get_vol_label(volume),NULL,UNKNOWN_OBJECT_TYPE,font,info_font_size,NULL, NULL, -1);

    }

    if (font != DEFAULT_FONT) {
        free(font);
    }

    return ctrl;
}

void sig_handler(int signo) {
    switch(signo) {
        case SIGHUP:
            log_info(MAIN_CTX, "SIGHUP received. Rereading config\n");
            init_config_file("ve301");
            if (default_theme) {
                free_theme(default_theme);
            }
            default_theme = get_config_theme ("Default");
            if (bluetooth_theme) {
                free_theme(bluetooth_theme);
            }
            bluetooth_theme = get_config_theme ("Bluetooth");
            if (spotify_theme) {
                free_theme(spotify_theme);
            }
            spotify_theme = get_config_theme ("Spotify");

            int y_offset = get_config_value_int("y_offset",DEFAULT_Y_OFFSET);
            int x_offset = get_config_value_int("x_offset",DEFAULT_Y_OFFSET);
            double angle_offset = get_config_value_double("angle_offset", DEFAULT_ANGLE_OFFSET);
            int radius_labels = get_config_value_int("radius_labels", DEFAULT_LABEL_RADIUS);
            int radius_scales_start = get_config_value_int("radius_scales_start", DEFAULT_SCALES_RADIUS_START);
            int radius_scales_end = get_config_value_int("radius_scales_end", DEFAULT_SCALES_RADIUS_START);

            ctrl->y_offset = y_offset;
            ctrl->x_offset = x_offset;
            ctrl->angle_offset = angle_offset;

            menu_ctrl_set_radii(ctrl,radius_labels,radius_scales_start,radius_scales_end);

            menu_ctrl_apply_theme (ctrl,default_theme);

            break;
        default:
            log_info(MAIN_CTX, "Bye with signal %d\n", signo);
            menu_ctrl_quit(ctrl);
            base_close();
            exit(0);
            break;
    }
}

int main(int argc, char **argv) {

    base_init("ve301", stderr, IR_LOG_LEVEL_DEBUG);

    log_info(MAIN_CTX, "Setting signals\n");
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGHUP, sig_handler);

    bt_init();

    log_info(MAIN_CTX, "Creating menu\n");
    ctrl = create_menu();

    log_info(MAIN_CTX, "Entering main loop\n");
    if (ctrl) {
        menu_ctrl_loop(ctrl);
        audio_disconnect();
        base_close();
        return EXIT_SUCCESS;
    } else {
        base_close();
        return EXIT_FAILURE;
    }

}

