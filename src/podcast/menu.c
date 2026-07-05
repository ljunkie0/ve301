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

#include "menu.h"

#include "podcast.h"
#include "../audio/song.h"
#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../menu/menu_item.h"
#include "../radio_app/radio_app.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define PODCAST_MENU_ITEMS_ON_SCALE_FACTOR 3
#define PODCAST_EPISODE_MENU_ITEMS_ON_SCALE_FACTOR 2

typedef struct podcast_menu_state {
    item_action *menu_action_listener;
    radio_app_touch_activity_fn *touch_activity;
    player *radio_player;
    unsigned int episode_limit;
    char font[MAX_CONFIG_LINE_LENGTH];
    int episode_font_size;
} podcast_menu_state;

static podcast_menu_state state = {0};

static int podcast_item_action(menu_event evt, menu *m, menu_item *item);

#define PODCAST_EPISODE_TITLE_LINE_CHARS 24
#define PODCAST_EPISODE_TITLE_MIN_SPLIT_CHARS 8

static size_t podcast_utf8_char_len(const char *s) {
    unsigned char c = (unsigned char) s[0];

    if (c >= 0xf8) {
        return 5;
    }
    if (c >= 0xf0) {
        return 4;
    }
    if (c >= 0xe0) {
        return 3;
    }
    if (c >= 0xc0) {
        return 2;
    }
    return 1;
}

static size_t podcast_utf8_count_range(const char *s, size_t bytes) {
    size_t chars = 0;
    size_t i = 0;

    while (s && i < bytes && s[i]) {
        size_t char_len = podcast_utf8_char_len(s + i);
        if (i + char_len > bytes) {
            break;
        }
        i += char_len;
        chars++;
    }

    return chars;
}

static size_t podcast_utf8_count(const char *s) {
    return s ? podcast_utf8_count_range(s, strlen(s)) : 0;
}

static size_t podcast_utf8_offset_for_chars(const char *s, size_t max_chars) {
    size_t chars = 0;
    size_t i = 0;

    while (s && s[i] && chars < max_chars) {
        i += podcast_utf8_char_len(s + i);
        chars++;
    }

    return i;
}

static char *podcast_copy_trimmed_range(const char *s, size_t bytes) {
    const char *start = s;
    const char *end = s + bytes;
    char *copy;

    while (start < end && isspace((unsigned char) *start)) {
        start++;
    }
    while (end > start && isspace((unsigned char) *(end - 1))) {
        end--;
    }

    copy = malloc((size_t) (end - start) + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, start, (size_t) (end - start));
    copy[end - start] = '\0';
    return copy;
}

static char *podcast_normalize_title(const char *title) {
    char *normalized;
    size_t len;
    size_t out = 0;
    int in_space = 1;

    if (!title || !title[0]) {
        return my_copystr("Episode");
    }

    len = strlen(title);
    normalized = malloc(len + 1);
    if (!normalized) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) title[i];
        if (isspace(c)) {
            if (!in_space) {
                normalized[out++] = ' ';
                in_space = 1;
            }
        } else {
            normalized[out++] = title[i];
            in_space = 0;
        }
    }

    if (out > 0 && normalized[out - 1] == ' ') {
        out--;
    }
    normalized[out] = '\0';

    return normalized;
}

static int podcast_title_split_char(char c) {
    return c == ' ' || c == '-' || c == ':' || c == '/';
}

static size_t podcast_title_find_split(const char *title, size_t max_chars) {
    size_t target = podcast_utf8_offset_for_chars(title, max_chars);
    size_t best = 0;

    for (size_t i = 0; title[i] && i <= target; i++) {
        if (podcast_title_split_char(title[i])
            && podcast_utf8_count_range(title, i) >= PODCAST_EPISODE_TITLE_MIN_SPLIT_CHARS) {
            best = i;
            if (title[i] != ' ') {
                best = i + 1;
            }
        }
    }

    if (best > 0) {
        return best;
    }

    return target;
}

static char *podcast_title_ellipsize(const char *title, size_t max_chars) {
    size_t chars = podcast_utf8_count(title);
    size_t keep;
    char *copy;

    if (chars <= max_chars) {
        return my_copystr(title);
    }

    keep = max_chars > 3 ? max_chars - 3 : max_chars;
    copy = podcast_copy_trimmed_range(title, podcast_utf8_offset_for_chars(title, keep));
    if (!copy) {
        return NULL;
    }

    char *ellipsized = my_catstr(copy, "...");
    free(copy);
    return ellipsized;
}

static void podcast_episode_title_lines(const char *title, char **line1, char **line2) {
    char *normalized = podcast_normalize_title(title);
    size_t title_chars;
    size_t split;

    *line1 = NULL;
    *line2 = NULL;

    if (!normalized) {
        return;
    }

    title_chars = podcast_utf8_count(normalized);
    if (title_chars <= PODCAST_EPISODE_TITLE_LINE_CHARS) {
        *line1 = normalized;
        return;
    }

    split = podcast_title_find_split(normalized, PODCAST_EPISODE_TITLE_LINE_CHARS);
    *line1 = podcast_copy_trimmed_range(normalized, split);
    *line2 = podcast_title_ellipsize(normalized + split, PODCAST_EPISODE_TITLE_LINE_CHARS);

    free(normalized);
}

static char *podcast_episode_menu_label(const podcast_episode *episode) {
    const char *title = episode && episode->name ? episode->name : "Episode";
    const char *date = episode && episode->date ? episode->date : "Unbekanntes Datum";
    char *line1;
    char *line2;
    char *label = NULL;

    podcast_episode_title_lines(title, &line1, &line2);
    if (!line1) {
        return my_cat3str("Episode", "\n", date);
    }

    if (line2 && line2[0]) {
        size_t len = strlen(line1) + strlen(line2) + strlen(date) + 3;
        label = malloc(len);
        if (label) {
            snprintf(label, len, "%s\n%s\n%s", line1, line2, date);
        }
    } else {
        label = my_cat3str(line1, "\n", date);
    }

    if (!label) {
        label = my_cat3str(line1, "\n", date);
    }

    free(line1);
    free(line2);
    return label;
}

static void podcast_set_submenu_geometry(menu *m) {
    if (!m) {
        return;
    }

    menu_set_no_items_on_scale(m,
                               PODCAST_EPISODE_MENU_ITEMS_ON_SCALE_FACTOR
                                   * menu_ctrl_get_n_o_items_on_scale(menu_get_ctrl(m)));
    menu_set_segments_per_item(m, 1);
}

static void podcast_fill_episode_menu(menu_item *item, podcast_feed *feed) {
    if (!item || !feed) {
        return;
    }

    menu *episode_menu = menu_item_get_sub_menu(item);
    if (!episode_menu) {
        return;
    }

    podcast_episode_list *episodes = podcast_episode_list_load(feed->url, state.episode_limit);
    menu_clear(episode_menu);
    podcast_set_submenu_geometry(episode_menu);
    menu_set_label(episode_menu, feed->name);

    if (!episodes || episodes->n_episodes == 0) {
        menu_item_new(episode_menu,
                      "Keine Episoden",
                      NULL,
                      NULL,
                      UNKNOWN_OBJECT_TYPE,
                      NULL,
                      0,
                      NULL,
                      NULL,
                      0);
        podcast_episode_list_free(episodes);
        return;
    }

    for (unsigned int i = 0; i < episodes->n_episodes; i++) {
        podcast_episode *episode = episodes->episodes[i];
        episodes->episodes[i] = NULL;
        char *label = podcast_episode_menu_label(episode);
        menu_item *episode_item = menu_item_new(episode_menu,
                                                label,
                                                NULL,
                                                episode,
                                                OBJ_TYPE_PODCAST_EPISODE,
                                                NULL,
                                                0,
                                                &podcast_item_action,
                                                NULL,
                                                0);
        free(label);
        menu_item_set_object_type(episode_item, OBJ_TYPE_PODCAST_EPISODE);
        menu_item_set_user_data(episode_item, episode);
    }

    podcast_episode_list_free(episodes);
    state.touch_activity(-1);
}

static void podcast_free_item_user_data(menu_item *item) {
    if (!item) {
        return;
    }

    switch (menu_item_get_object_type(item)) {
    case OBJ_TYPE_PODCAST_FEED:
        podcast_feed_free((podcast_feed *) menu_item_get_user_data(item));
        menu_item_set_user_data(item, NULL);
        break;
    case OBJ_TYPE_PODCAST_EPISODE:
        podcast_episode_free((podcast_episode *) menu_item_get_user_data(item));
        menu_item_set_user_data(item, NULL);
        break;
    default:
        break;
    }
}

static int podcast_item_action(menu_event evt, menu *m, menu_item *item) {
    if (state.touch_activity) {
        state.touch_activity(-1);
    }

    if (evt == DISPOSE) {
        podcast_free_item_user_data(item);
        return 0;
    }

    if (evt != ACTIVATE && evt != ACTIVATE_1) {
        return state.menu_action_listener ? state.menu_action_listener(evt, m, item) : 0;
    }

    if (!item) {
        return state.menu_action_listener ? state.menu_action_listener(evt, m, item) : 0;
    }

    int object_type = menu_item_get_object_type(item);
    if (object_type == OBJ_TYPE_PODCAST_FEED) {
        podcast_fill_episode_menu(item, (podcast_feed *) menu_item_get_user_data(item));
        return 0;
    }

    if (object_type == OBJ_TYPE_PODCAST_EPISODE) {
        podcast_episode *episode = (podcast_episode *) menu_item_get_user_data(item);
        if (!episode || !episode->url) {
            return 0;
        }

        song *s = song_new(unknown_song_id, episode->url, episode->name, episode->name);
        if (!player_playback_start(state.radio_player, s)) {
            log_error(MAIN_CTX, "Podcast: could not play episode %s\n", episode->name);
            song_free(s);
        }
        return 0;
    }

    return state.menu_action_listener ? state.menu_action_listener(evt, m, item) : 0;
}

void podcast_attach_navigation_menu(const radio_app_navigation_context *ctx) {
    if (!ctx || !ctx->config || !ctx->config->podcast_enabled) {
        return;
    }

    state.menu_action_listener = menu_ctrl_get_item_action(ctx->ctrl);
    state.touch_activity = ctx->touch_activity;
    state.radio_player = ctx->radio_player;
    state.episode_limit = ctx->config->podcast_episode_limit > 0
                              ? (unsigned int) ctx->config->podcast_episode_limit
                              : 50;
    snprintf(state.font, sizeof(state.font), "%s", ctx->config->font);
    state.episode_font_size = ctx->config->podcast_episode_font_size;

    podcast_feed_list *feeds = podcast_feed_list_load(ctx->config->podcast_feeds_file);

    menu *podcast_menu = menu_new(ctx->ctrl, 1, NULL, 0, &podcast_item_action, NULL, 0);
    menu_set_label(podcast_menu, "Podcasts");
    menu_set_no_items_on_scale(podcast_menu, PODCAST_MENU_ITEMS_ON_SCALE_FACTOR);
    menu_set_segments_per_item(podcast_menu, 2);

    if (!feeds || feeds->n_feeds == 0) {
        menu_item_new(podcast_menu,
                      "Keine Podcasts",
                      NULL,
                      NULL,
                      UNKNOWN_OBJECT_TYPE,
                      NULL,
                      0,
                      NULL,
                      NULL,
                      0);
        podcast_feed_list_free(feeds);
        menu_add_sub_menu(ctx->nav_menu, "Podcasts", podcast_menu, NULL);
        return;
    }

    for (unsigned int i = 0; i < feeds->n_feeds; i++) {
        podcast_feed *feed = feeds->feeds[i];
        feeds->feeds[i] = NULL;

        menu *feed_menu = menu_new(ctx->ctrl,
                                   1,
                                   state.font,
                                   state.episode_font_size,
                                   &podcast_item_action,
                                   state.font,
                                   state.episode_font_size);
        menu_set_label(feed_menu, feed->name);
        podcast_set_submenu_geometry(feed_menu);

        menu_item *feed_item = menu_add_sub_menu(podcast_menu,
                                                 feed->name,
                                                 feed_menu,
                                                 &podcast_item_action);
        menu_item_set_object_type(feed_item, OBJ_TYPE_PODCAST_FEED);
        menu_item_set_user_data(feed_item, feed);
    }

    podcast_feed_list_free(feeds);
    menu_add_sub_menu(ctx->nav_menu, "Podcasts", podcast_menu, NULL);
}
