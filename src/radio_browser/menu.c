#include "menu.h"
#include "../audio/player.h"
#include "radio_browser.h"
#include "../audio/song.h"
#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../menu/menu_item.h"
#include "../radio_app/radio_app.h"
#include <stdlib.h>
#include <stdio.h>

#define RADIO_MENU_ITEMS_ON_SCALE_FACTOR 3

struct __radio_browser_config {
    char radio_browser_countrycode[MAX_CONFIG_LINE_LENGTH];
    char radio_browser_server[MAX_CONFIG_LINE_LENGTH];
    char radio_browser_user_agent[MAX_CONFIG_LINE_LENGTH];
    int radio_browser_station_limit;
    int radio_browser_category_limit;
    int radio_browser_language_limit;
    int radio_browser_station_font_size;
    char font[MAX_CONFIG_LINE_LENGTH];
    radio_app_touch_activity_fn *radio_app_touch_activity;
    item_action *menu_action_listener;
    player *radio_player;
};

struct __radio_browser_config radio_browser_config;

char *radio_browser_station_label(const radio_browser_station *station) {
    if (!station) {
        return my_copystr("Unknown");
    }

    if (station->name && station->name[0]) {
        return my_copystr(station->name);
    }

    if (station->url_resolved && station->url_resolved[0]) {
        return my_copystr(station->url_resolved);
    }

    if (station->url && station->url[0]) {
        return my_copystr(station->url);
    }

    return my_copystr("Unknown");
}

void radio_browser_fill_station_menu(menu *station_menu, radio_browser_station_list *list);
void radio_browser_fill_entry_menu(menu *entry_menu,
                                   radio_browser_entry_list *list,
                                   int object_type);
void radio_browser_open_station_menu(menu_item *item,
                                     radio_browser_station_list *list,
                                     const char *label);
int radio_browser_item_action(menu_event evt, menu *m, menu_item *item);
int add_to_playlist_action(menu_event evt, menu *m, menu_item *item);

void radio_browser_play_station(radio_browser_station *station) {
    if (!station) {
        return;
    }

    const char *station_name = station->name && station->name[0] ? station->name : "Unknown";
    char *stream_url = radio_browser_resolve_stream_url(station);
    if (!stream_url || !stream_url[0]) {
        log_error(MAIN_CTX, "Radio Browser: could not resolve stream URL for %s\n", station_name);
        free(stream_url);
        return;
    }

    song *s = song_new(unknown_song_id, stream_url, NULL, station_name);
    if (!player_playback_start(radio_browser_config.radio_player, s)) {
        log_error(MAIN_CTX, "Radio Browser: could not play station %s\n", station_name);
        song_free(s);
    }
    free(stream_url);
}

menu_item *radio_browser_add_station_item(menu *station_menu, radio_browser_station *station) {
    char *label = radio_browser_station_label(station);
    menu_item *station_item = menu_item_new(station_menu,
                                            label,
                                            NULL,
                                            station,
                                            OBJ_TYPE_RADIO_BROWSER_STATION,
                                            NULL,
                                            0,
                                            &radio_browser_item_action,
                                            NULL,
                                            0);
    menu_item_set_object_type(station_item, OBJ_TYPE_RADIO_BROWSER_STATION);
    menu_item_set_user_data(station_item, station);
    free(label);
    return station_item;
}

void radio_browser_fill_station_menu(menu *station_menu, radio_browser_station_list *list) {
    if (!station_menu) {
        return;
    }

    menu_clear(station_menu);
    menu_set_no_items_on_scale(station_menu,
                               RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(menu_get_ctrl(station_menu)));
    menu_set_segments_per_item(station_menu, 1);

    if (!list || list->n_stations == 0) {
        menu_item_new(
            station_menu, "Keine Sender", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, 0);
        return;
    }

    for (unsigned int i = 0; i < list->n_stations; i++) {
        radio_browser_station *station = radio_browser_station_clone(list->stations[i]);
        if (station) {
            radio_browser_add_station_item(station_menu, station);
        }
    }
}

void radio_browser_fill_entry_menu(menu *entry_menu,
                                   radio_browser_entry_list *list,
                                   int object_type) {
    if (!entry_menu) {
        return;
    }

    menu_clear(entry_menu);
    menu_set_no_items_on_scale(entry_menu,
                               RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(menu_get_ctrl(entry_menu)));
    menu_set_segments_per_item(entry_menu, 1);

    if (!list || list->n_entries == 0) {
        menu_item_new(
            entry_menu, "Keine Treffer", NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, 0, NULL, NULL, 0);
        return;
    }

    for (unsigned int i = 0; i < list->n_entries; i++) {
        radio_browser_entry *entry = radio_browser_entry_clone(list->entries[i]);
        if (!entry) {
            continue;
        }
        menu *sub_menu = menu_new(menu_get_ctrl(entry_menu),
                                  3,
                                  radio_browser_config.font,
                                  radio_browser_config.radio_browser_station_font_size,
                                  &radio_browser_item_action,
                                  NULL,
                                  0);
        menu_set_label(sub_menu, entry->label);
        menu_set_no_items_on_scale(sub_menu,
                                   RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                       * menu_ctrl_get_n_o_items_on_scale(
                                           menu_get_ctrl(entry_menu)));
        menu_set_segments_per_item(sub_menu, 1);
        menu_item *entry_item = menu_add_sub_menu(entry_menu,
                                                  entry->label,
                                                  sub_menu,
                                                  &radio_browser_item_action);
        menu_item_set_object_type(entry_item, object_type);
        menu_item_set_user_data(entry_item, entry);
    }
}

void radio_browser_open_station_menu(menu_item *item,
                                     radio_browser_station_list *list,
                                     const char *label) {
    if (!item || !menu_item_get_sub_menu(item)) {
        return;
    }

    menu *station_menu = menu_item_get_sub_menu(item);
    radio_browser_fill_station_menu(station_menu, list);
    if (label) {
        menu_set_label(station_menu, label);
    }
}

int radio_browser_item_action(menu_event evt, menu *m, menu_item *item) {
    if (log_level_enabled(MAIN_CTX, IR_LOG_LEVEL_CONFIG)) {
        char *label = item ? menu_item_get_label(item) : "";
        log_config(MAIN_CTX, "action(%d, %p, %s)\n", evt, item, label);
    }

    radio_browser_config.radio_app_touch_activity(-1);

    (void) m;
    if (!item) {
        return radio_browser_config.menu_action_listener(evt, m, item);
    }

    int object_type = menu_item_get_object_type(item);
    if (evt == DISPOSE) {
        switch (object_type) {
        case OBJ_TYPE_RADIO_BROWSER_TAG:
        case OBJ_TYPE_RADIO_BROWSER_LANGUAGE:
            radio_browser_entry_free((radio_browser_entry *) menu_item_get_user_data(item));
            menu_item_set_user_data(item, NULL);
            break;
        case OBJ_TYPE_RADIO_BROWSER_STATION:
            radio_browser_station_free((radio_browser_station *) menu_item_get_user_data(item));
            menu_item_set_user_data(item, NULL);
            break;
        default:
            break;
        }
        return 0;
    }

    if (evt != ACTIVATE && evt != ACTIVATE_1) {
        return radio_browser_config.menu_action_listener(evt, m, item);
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_LOCAL) {
        radio_browser_station_list *stations
            = radio_browser_get_local_stations(radio_browser_config.radio_browser_countrycode,
                                               radio_browser_config.radio_browser_station_limit);
        radio_browser_open_station_menu(item, stations, "Lokal");
        radio_browser_station_list_free(stations);
        return 0;
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_TAG_ROOT) {
        radio_browser_entry_list *tags = radio_browser_get_tags(
            radio_browser_config.radio_browser_category_limit);
        radio_browser_fill_entry_menu(menu_item_get_sub_menu(item),
                                      tags,
                                      OBJ_TYPE_RADIO_BROWSER_TAG);
        radio_browser_entry_list_free(tags);
        return 0;
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_LANGUAGE_ROOT) {
        radio_browser_entry_list *languages = radio_browser_get_languages(
            radio_browser_config.radio_browser_language_limit);
        radio_browser_fill_entry_menu(menu_item_get_sub_menu(item),
                                      languages,
                                      OBJ_TYPE_RADIO_BROWSER_LANGUAGE);
        radio_browser_entry_list_free(languages);
        return 0;
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_TAG) {
        radio_browser_entry *entry = (radio_browser_entry *) menu_item_get_user_data(item);
        radio_browser_station_list *stations
            = entry ? radio_browser_get_stations_by_tag(entry->value,
                                                        radio_browser_config
                                                            .radio_browser_station_limit)
                    : NULL;
        radio_browser_open_station_menu(item, stations, entry ? entry->label : NULL);
        radio_browser_station_list_free(stations);
        return 0;
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_LANGUAGE) {
        radio_browser_entry *entry = (radio_browser_entry *) menu_item_get_user_data(item);
        radio_browser_station_list *stations
            = entry ? radio_browser_get_stations_by_language(entry->value,
                                                             radio_browser_config
                                                                 .radio_browser_station_limit)
                    : NULL;
        radio_browser_open_station_menu(item, stations, entry ? entry->label : NULL);
        radio_browser_station_list_free(stations);
        return 0;
    }

    if (object_type == OBJ_TYPE_RADIO_BROWSER_STATION) {
        radio_browser_play_station((radio_browser_station *) menu_item_get_user_data(item));
        return 0;
    }

    return 0;
}

static menu *radio_browser_menu_new(menu_ctrl *ctrl,
                                    radio_app_touch_activity_fn *radio_app_touch_activity,
                                    const char *server,
                                    const char *user_agent,
                                    const char *country_code,
                                    int station_limit,
                                    int category_limit,
                                    int language_limit,
                                    const char *font,
                                    int station_font_size,
                                    player *radio_player) {
    radio_browser_config.menu_action_listener = menu_ctrl_get_item_action(ctrl);
    radio_browser_config.radio_app_touch_activity = radio_app_touch_activity;
    snprintf(radio_browser_config.radio_browser_countrycode,
             sizeof(radio_browser_config.radio_browser_countrycode),
             "%s",
             country_code);
    radio_browser_config.radio_browser_station_limit = station_limit;
    radio_browser_config.radio_browser_category_limit = category_limit;
    radio_browser_config.radio_browser_language_limit = language_limit;
    snprintf(radio_browser_config.font, sizeof(radio_browser_config.font), "%s", font);
    radio_browser_config.radio_browser_station_font_size = station_font_size;
    radio_browser_config.radio_player = radio_player;

    radio_browser_init(user_agent, server);

    menu *radio_browser_menu = menu_new(ctrl, 1, NULL, 0, NULL, NULL, 0);
    menu_set_label(radio_browser_menu, "Radio Browser");
    menu_set_no_items_on_scale(radio_browser_menu, 3);
    menu_set_segments_per_item(radio_browser_menu, 2);

    menu *radio_browser_local_menu = menu_new(ctrl,
                                              3,
                                              radio_browser_config.font,
                                              radio_browser_config.radio_browser_station_font_size,
                                              &radio_browser_item_action,
                                              NULL,
                                              0);
    menu_set_label(radio_browser_local_menu, "Lokal");
    menu_set_no_items_on_scale(radio_browser_local_menu,
                               RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(ctrl));
    menu_set_segments_per_item(radio_browser_local_menu, 1);
    menu_item *local_item = menu_add_sub_menu(radio_browser_menu,
                                              "Lokal",
                                              radio_browser_local_menu,
                                              &radio_browser_item_action);
    menu_item_set_object_type(local_item, OBJ_TYPE_RADIO_BROWSER_LOCAL);

    menu *radio_browser_tag_menu = menu_new(ctrl, 3, NULL, 0, &radio_browser_item_action, NULL, 0);
    menu_set_label(radio_browser_tag_menu, "Kategorien");
    menu_set_no_items_on_scale(radio_browser_tag_menu,
                               RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(ctrl));
    menu_set_segments_per_item(radio_browser_tag_menu, 1);
    menu_item *tag_item = menu_add_sub_menu(radio_browser_menu,
                                            "Kategorien",
                                            radio_browser_tag_menu,
                                            &radio_browser_item_action);
    menu_item_set_object_type(tag_item, OBJ_TYPE_RADIO_BROWSER_TAG_ROOT);

    menu *radio_browser_language_menu
        = menu_new(ctrl, 3, NULL, 0, &radio_browser_item_action, NULL, 0);
    menu_set_label(radio_browser_language_menu, "Sprachen");
    menu_set_no_items_on_scale(radio_browser_language_menu,
                               RADIO_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(ctrl));
    menu_set_segments_per_item(radio_browser_language_menu, 1);
    menu_item *language_item = menu_add_sub_menu(radio_browser_menu,
                                                 "Sprachen",
                                                 radio_browser_language_menu,
                                                 &radio_browser_item_action);
    menu_item_set_object_type(language_item, OBJ_TYPE_RADIO_BROWSER_LANGUAGE_ROOT);

    return radio_browser_menu;
}

void radio_browser_attach_navigation_menu(const radio_app_navigation_context *ctx) {
    if (!ctx || !ctx->config || !ctx->config->radio_browser_enabled) {
        return;
    }

    menu *radio_browser_menu = radio_browser_menu_new(ctx->ctrl,
                                                      ctx->touch_activity,
                                                      ctx->config->radio_browser_server,
                                                      ctx->config->radio_browser_user_agent,
                                                      ctx->config->radio_browser_countrycode,
                                                      ctx->config->radio_browser_station_limit,
                                                      ctx->config->radio_browser_category_limit,
                                                      ctx->config->radio_browser_language_limit,
                                                      ctx->config->font,
                                                      ctx->config->radio_browser_station_font_size,
                                                      ctx->radio_player);
    menu_add_sub_menu(ctx->nav_menu, "Radio Browser", radio_browser_menu, NULL);
}

void radio_browser_menu_close(void) {
    radio_browser_cleanup();
}
