/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 */

#include "menu_web.h"

#ifdef MENU_WEB

#include "../../base/config.h"
#include "../../base/log_contexts.h"
#include "../../base/logging.h"
#include "../menu_ctrl_priv.h"
#include "../menu_item.h"
#include "../menu_menu.h"
#include <mongoose.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MENU_WEB_DEFAULT_LISTEN
#define MENU_WEB_DEFAULT_LISTEN "http://0.0.0.0:8000"
#endif

typedef struct menu_web_buffer {
    char *data;
    size_t len;
    size_t cap;
} menu_web_buffer;

struct menu_web {
    menu_ctrl *ctrl;
    struct mg_mgr mgr;
    char *listen_url;
};

#define MENU_WEB_MONGOOSE_LOG_BUFFER_SIZE 1024

typedef struct menu_web_mongoose_log {
    char line[MENU_WEB_MONGOOSE_LOG_BUFFER_SIZE];
    size_t len;
} menu_web_mongoose_log;

static menu_web_mongoose_log s_mongoose_log;

static int menu_web_mongoose_level_from_line(const char *line) {
    const char *p = strchr(line, ' ');
    if (!p) {
        return MG_LL_DEBUG;
    }

    while (*p == ' ') {
        p++;
    }

    if (*p >= '0' && *p <= '4') {
        return *p - '0';
    }

    return MG_LL_DEBUG;
}

static void menu_web_mongoose_log_line(const char *line) {
    switch (menu_web_mongoose_level_from_line(line)) {
    case MG_LL_ERROR:
        log_error(MENU_CTX, "mongoose: %s\n", line);
        break;
    case MG_LL_INFO:
        log_info(MENU_CTX, "mongoose: %s\n", line);
        break;
    case MG_LL_VERBOSE:
        log_config(MENU_CTX, "mongoose: %s\n", line);
        break;
    case MG_LL_DEBUG:
    default:
        log_debug(MENU_CTX, "mongoose: %s\n", line);
        break;
    }
}

static void menu_web_mongoose_log_char(char ch, void *param) {
    menu_web_mongoose_log *state = param ? (menu_web_mongoose_log *) param : &s_mongoose_log;

    if (ch == '\r') {
        return;
    }

    if (ch == '\n') {
        if (state->len > 0) {
            state->line[state->len] = '\0';
            menu_web_mongoose_log_line(state->line);
            state->len = 0;
        }
        return;
    }

    if (state->len + 1 >= sizeof(state->line)) {
        state->line[state->len] = '\0';
        menu_web_mongoose_log_line(state->line);
        state->len = 0;
    }

    state->line[state->len++] = ch;
}

static int menu_web_mongoose_level_from_menu_log(void) {
    int level = get_log_level(MENU_CTX);

    if (level <= IR_LOG_LEVEL_OFF) {
        return MG_LL_NONE;
    }
    if (level < IR_LOG_LEVEL_INFO) {
        return MG_LL_ERROR;
    }
    if (level < IR_LOG_LEVEL_DEBUG) {
        return MG_LL_INFO;
    }
    if (level < IR_LOG_LEVEL_TRACE) {
        return MG_LL_DEBUG;
    }
    return MG_LL_VERBOSE;
}

static void menu_web_configure_mongoose_logging(void) {
    s_mongoose_log.len = 0;
    mg_log_set_fn(menu_web_mongoose_log_char, &s_mongoose_log);
    mg_log_set(menu_web_mongoose_level_from_menu_log());
}

static void menu_web_buffer_free(menu_web_buffer *buf) {
    if (buf) {
        free(buf->data);
        buf->data = NULL;
        buf->len = 0;
        buf->cap = 0;
    }
}

static int menu_web_buffer_reserve(menu_web_buffer *buf, size_t extra) {
    size_t needed = buf->len + extra + 1;
    if (needed <= buf->cap) {
        return 1;
    }

    size_t cap = buf->cap ? buf->cap : 1024;
    while (cap < needed) {
        cap *= 2;
    }

    char *data = realloc(buf->data, cap);
    if (!data) {
        return 0;
    }

    buf->data = data;
    buf->cap = cap;
    return 1;
}

static int menu_web_buffer_append_len(menu_web_buffer *buf, const char *value, size_t len) {
    if (!menu_web_buffer_reserve(buf, len)) {
        return 0;
    }
    memcpy(buf->data + buf->len, value, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 1;
}

static int menu_web_buffer_append(menu_web_buffer *buf, const char *value) {
    return menu_web_buffer_append_len(buf, value, strlen(value));
}

static int menu_web_buffer_appendf(menu_web_buffer *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (len < 0 || !menu_web_buffer_reserve(buf, (size_t) len)) {
        va_end(ap2);
        return 0;
    }

    vsnprintf(buf->data + buf->len, buf->cap - buf->len, fmt, ap2);
    va_end(ap2);
    buf->len += (size_t) len;
    return 1;
}

static int menu_web_buffer_append_json_string(menu_web_buffer *buf, const char *value) {
    if (!menu_web_buffer_append(buf, "\"")) {
        return 0;
    }

    if (value) {
        const unsigned char *p = (const unsigned char *) value;
        while (*p) {
            char tmp[8];
            switch (*p) {
            case '"':
                if (!menu_web_buffer_append(buf, "\\\"")) return 0;
                break;
            case '\\':
                if (!menu_web_buffer_append(buf, "\\\\")) return 0;
                break;
            case '\b':
                if (!menu_web_buffer_append(buf, "\\b")) return 0;
                break;
            case '\f':
                if (!menu_web_buffer_append(buf, "\\f")) return 0;
                break;
            case '\n':
                if (!menu_web_buffer_append(buf, "\\n")) return 0;
                break;
            case '\r':
                if (!menu_web_buffer_append(buf, "\\r")) return 0;
                break;
            case '\t':
                if (!menu_web_buffer_append(buf, "\\t")) return 0;
                break;
            default:
                if (*p < 0x20) {
                    snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
                    if (!menu_web_buffer_append(buf, tmp)) return 0;
                } else if (!menu_web_buffer_append_len(buf, (const char *) p, 1)) {
                    return 0;
                }
            }
            p++;
        }
    }

    return menu_web_buffer_append(buf, "\"");
}

static int menu_web_uri_eq(struct mg_str uri, const char *value) {
    size_t len = strlen(value);
    return uri.len == len && !strncmp(uri.buf, value, len);
}

static void menu_web_append_item(menu_web_buffer *buf,
                                 menu_item *item,
                                 const char *path,
                                 menu *current,
                                 unsigned long long revision,
                                 int *first);

static void menu_web_append_menu_items(menu_web_buffer *buf,
                                       menu *m,
                                       const char *path,
                                       menu *current,
                                       unsigned long long revision) {
    int first = 1;
    menu_web_buffer_append(buf, "[");
    for (int i = 0; i <= menu_get_max_id(m); i++) {
        menu_item *item = menu_get_item(m, i);
        if (!item || !menu_item_get_visible(item)) {
            continue;
        }

        char child_path[256];
        snprintf(child_path, sizeof(child_path), "%s/%d", path, i);
        menu_web_append_item(buf, item, child_path, current, revision, &first);
    }
    menu_web_buffer_append(buf, "]");
}

static void menu_web_append_icon_url(menu_web_buffer *buf, menu_item *item, const char *path) {
    const char *icon = menu_item_get_icon(item);
    char icon_url[320];

    if (icon && icon[0]) {
        snprintf(icon_url, sizeof(icon_url), "/api/menu/icon?path=%s", path);
        menu_web_buffer_append_json_string(buf, icon_url);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
}

static unsigned int menu_web_style_version(menu_ctrl *ctrl) {
    return ctrl ? ctrl->style_version : 0;
}

static void menu_web_append_versioned_path_url(menu_web_buffer *buf,
                                               const char *url_path,
                                               const char *asset_path,
                                               const char *path,
                                               unsigned long long revision) {
    char url[360];

    if (asset_path && asset_path[0]) {
        snprintf(url, sizeof(url), "%s?path=%s&v=%016llx", url_path, path, revision);
        menu_web_buffer_append_json_string(buf, url);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
}

static void menu_web_append_font_path_url(menu_web_buffer *buf,
                                          const char *font_path,
                                          const char *path,
                                          unsigned long long revision) {
    menu_web_append_versioned_path_url(buf, "/api/menu/font", font_path, path, revision);
}

static void menu_web_append_font_url(menu_web_buffer *buf,
                                     menu_item *item,
                                     const char *path,
                                     unsigned long long revision) {
    menu_web_append_font_path_url(buf, menu_item_get_effective_font_path(item), path, revision);
}

static void menu_web_append_menu_font_url(menu_web_buffer *buf,
                                          menu *m,
                                          const char *path,
                                          unsigned long long revision) {
    menu_web_append_font_path_url(buf, menu_get_effective_font_path(m), path, revision);
}

static void menu_web_append_background_url(menu_web_buffer *buf,
                                           menu *m,
                                           const char *path,
                                           unsigned long long revision) {
    menu_web_append_versioned_path_url(buf,
                                       "/api/menu/background",
                                       menu_get_effective_background_path(m),
                                       path,
                                       revision);
}

static void menu_web_hash_bytes(unsigned long long *hash, const void *data, size_t len) {
    const unsigned char *p = data;

    for (size_t i = 0; i < len; i++) {
        *hash ^= p[i];
        *hash *= 1099511628211ULL;
    }
}

static void menu_web_hash_int(unsigned long long *hash, int value) {
    menu_web_hash_bytes(hash, &value, sizeof(value));
}

static void menu_web_hash_menu(unsigned long long *hash, menu *m);

static void menu_web_hash_item(unsigned long long *hash, menu_item *item) {
    menu *sub_menu = menu_item_get_sub_menu(item);

    menu_web_hash_int(hash, menu_item_get_id(item));
    menu_web_hash_int(hash, menu_item_get_object_type(item));
    menu_web_hash_int(hash, sub_menu ? 1 : 0);
    menu_web_hash_int(hash, menu_item_get_icon(item) ? 1 : 0);

    if (sub_menu) {
        menu_web_hash_menu(hash, sub_menu);
    }
}

static void menu_web_hash_menu(unsigned long long *hash, menu *m) {
    menu_web_hash_int(hash, menu_is_transient(m));
    menu_web_hash_int(hash, menu_get_max_id(m));

    for (int i = 0; i <= menu_get_max_id(m); i++) {
        menu_item *item = menu_get_item(m, i);
        menu_web_hash_int(hash, item ? 1 : 0);
        if (item) {
            menu_web_hash_int(hash, menu_item_get_visible(item));
            if (menu_item_get_visible(item)) {
                menu_web_hash_item(hash, item);
            }
        }
    }
}

static unsigned long long menu_web_structure_hash(menu_ctrl *ctrl) {
    unsigned long long hash = 1469598103934665603ULL;

    menu_web_hash_int(&hash, menu_ctrl_get_root_count(ctrl));
    for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
        menu *root = menu_ctrl_get_root_at(ctrl, r);
        menu_web_hash_int(&hash, root ? 1 : 0);
        if (root) {
            menu_web_hash_menu(&hash, root);
        }
    }

    return hash;
}

static void menu_web_append_color(menu_web_buffer *buf, const SDL_Color *color) {
    if (color) {
        menu_web_buffer_appendf(buf, "\"#%02x%02x%02x\"", color->r, color->g, color->b);
    } else {
        menu_web_buffer_append(buf, "null");
    }
}

static const SDL_Color *menu_web_item_color(menu_item *item, menu *current) {
    menu *m = menu_item_get_menu(item);
    int id = menu_item_get_id(item);

    if (menu_get_active_id(m) == id) {
        return menu_get_effective_active_color(m);
    }
    if (m == current && menu_get_current_id(m) == id) {
        return menu_get_effective_selected_color(m);
    }
    return menu_get_effective_default_color(m);
}

static menu *menu_web_current_root(menu_ctrl *ctrl, int *index_out) {
    menu *first = NULL;
    menu *fallback = NULL;
    int first_index = -1;
    int fallback_index = -1;

    for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
        menu *root = menu_ctrl_get_root_at(ctrl, r);
        if (!root) {
            continue;
        }
        if (root == menu_ctrl_get_current_transient(ctrl)) {
            *index_out = r;
            return root;
        }
        if (!first) {
            first = root;
            first_index = r;
        }
        if (!fallback && !menu_is_transient(root)) {
            fallback = root;
            fallback_index = r;
        }
    }

    *index_out = fallback ? fallback_index : first_index;
    return fallback ? fallback : first;
}

static int menu_web_resolve_path(menu_ctrl *ctrl,
                                 const char *path,
                                 menu **menu_out,
                                 menu_item **item_out);
static int menu_web_query_value(struct mg_str query,
                                const char *name,
                                char *buffer,
                                size_t buffer_len);

static void menu_web_append_compact_item(menu_web_buffer *buf,
                                         menu_item *item,
                                         const char *path,
                                         menu *current,
                                         unsigned int revision,
                                         int *first) {
    menu *m = menu_item_get_menu(item);
    menu *sub_menu = menu_item_get_sub_menu(item);
    int id = menu_item_get_id(item);

    if (!*first) {
        menu_web_buffer_append(buf, ",");
    }
    *first = 0;

    menu_web_buffer_append(buf, "{\"path\":");
    menu_web_buffer_append_json_string(buf, path);
    menu_web_buffer_append(buf, ",\"label\":");
    menu_web_buffer_append_json_string(buf, menu_item_get_label(item));
    menu_web_buffer_append(buf, ",\"icon\":");
    menu_web_append_icon_url(buf, item, path);
    menu_web_buffer_append(buf, ",\"font\":");
    menu_web_append_font_url(buf, item, path, revision);
    menu_web_buffer_appendf(buf,
                            ",\"font_size\":%d,\"submenu\":%s,\"current\":%s,\"active\":%s,\"color\":",
                            menu_item_get_effective_font_size(item),
                            sub_menu ? "true" : "false",
                            m == current && menu_get_current_id(m) == id ? "true" : "false",
                            menu_get_active_id(m) == id ? "true" : "false");
    menu_web_append_color(buf, menu_web_item_color(item, current));
    menu_web_buffer_append(buf, "}");
}

static void menu_web_append_compact_menu(menu_web_buffer *buf,
                                         menu *m,
                                         const char *path,
                                         menu *current,
                                         unsigned int revision,
                                         int *first) {
    for (int i = 0; i <= menu_get_max_id(m); i++) {
        menu_item *item = menu_get_item(m, i);
        char child_path[256];

        if (!item || !menu_item_get_visible(item)) {
            continue;
        }
        snprintf(child_path, sizeof(child_path), "%s/%d", path, i);
        menu_web_append_compact_item(buf, item, child_path, current, revision, first);
    }
}

static void menu_web_append_status_items(menu_web_buffer *buf,
                                         menu *m,
                                         const char *path,
                                         menu *current,
                                         unsigned int revision,
                                         int *first) {
    for (int i = 0; i <= menu_get_max_id(m); i++) {
        menu_item *item = menu_get_item(m, i);
        menu *sub_menu;
        char child_path[256];

        if (!item || !menu_item_get_visible(item)) {
            continue;
        }
        snprintf(child_path, sizeof(child_path), "%s/%d", path, i);
        menu_web_append_compact_item(buf, item, child_path, current, revision, first);
        sub_menu = menu_item_get_sub_menu(item);
        if (sub_menu) {
            menu_web_append_status_items(buf, sub_menu, child_path, current, revision, first);
        }
    }
}

static void menu_web_append_compact_root(menu_web_buffer *buf,
                                         menu_ctrl *ctrl,
                                         menu *root,
                                         int root_index,
                                         unsigned int revision,
                                         int *first) {
    char path[32];

    if (!*first) {
        menu_web_buffer_append(buf, ",");
    }
    *first = 0;
    snprintf(path, sizeof(path), "%d", root_index);
    menu_web_buffer_append(buf, "{\"path\":");
    menu_web_buffer_append_json_string(buf, path);
    menu_web_buffer_append(buf, ",\"label\":");
    menu_web_buffer_append_json_string(buf, menu_get_label(root));
    menu_web_buffer_append(buf, ",\"icon\":null,\"font\":");
    menu_web_append_menu_font_url(buf, root, path, revision);
    menu_web_buffer_appendf(buf,
                            ",\"font_size\":%d,\"submenu\":true,\"current\":%s,\"active\":%s,\"color\":",
                            menu_get_effective_font_size(root),
                            root == menu_ctrl_get_current(ctrl) ? "true" : "false",
                            root == menu_ctrl_get_active(ctrl) ? "true" : "false");
    menu_web_append_color(buf, menu_get_effective_default_color(root));
    menu_web_buffer_append(buf, "}");
}

static void menu_web_append_state_json(menu_web_buffer *buf,
                                       menu_web *web,
                                       const char *view_path) {
    menu_ctrl *ctrl = web->ctrl;
    unsigned int version = menu_web_style_version(ctrl);
    unsigned long long structure_hash = menu_web_structure_hash(ctrl);
    menu *current = menu_ctrl_get_current(ctrl);
    menu *view_menu = NULL;
    menu_item *view_item = NULL;
    menu *theme_root;
    int theme_root_index = -1;
    int view_valid = 1;
    int first = 1;
    int regular_roots = 0;
    int regular_root_index = -1;
    char root_path[32];

    if (view_path && view_path[0]) {
        if (!menu_web_resolve_path(ctrl, view_path, &view_menu, &view_item)) {
            view_valid = 0;
        } else if (view_item) {
            view_menu = menu_item_get_sub_menu(view_item);
            if (!view_menu) {
                view_valid = 0;
            }
        }
    } else {
        for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
            menu *root = menu_ctrl_get_root_at(ctrl, r);
            if (root && !menu_is_transient(root)) {
                regular_roots++;
                regular_root_index = r;
            }
        }
        if (regular_roots == 1) {
            view_menu = menu_ctrl_get_root_at(ctrl, regular_root_index);
        }
    }

    menu_web_buffer_appendf(buf,
                            "{\"style_version\":%u,\"structure_version\":\"%016llx\",\"view_valid\":%s,\"title\":",
                            version,
                            structure_hash,
                            view_valid ? "true" : "false");
    menu_web_buffer_append_json_string(
        buf,
        view_item ? menu_item_get_label(view_item)
                  : (view_path && view_path[0] && view_menu
                     ? menu_get_label(view_menu)
                     : "Menu"));
    menu_web_buffer_append(buf, ",\"nav\":[");
    if (view_valid) {
        if (view_menu) {
            const char *path = view_path && view_path[0] ? view_path : root_path;
            if (!view_path || !view_path[0]) {
                snprintf(root_path, sizeof(root_path), "%d", regular_root_index);
            }
            menu_web_append_compact_menu(buf, view_menu, path, current, version, &first);
        } else {
            for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
                menu *root = menu_ctrl_get_root_at(ctrl, r);
                if (root && !menu_is_transient(root)) {
                    menu_web_append_compact_root(buf, ctrl, root, r, version, &first);
                }
            }
        }
    }
    menu_web_buffer_append(buf, "],\"status\":[");
    first = 1;
    menu *status_root = menu_ctrl_get_current_transient(ctrl);
    if (status_root) {
        int status_index = -1;
        for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
            if (menu_ctrl_get_root_at(ctrl, r) == status_root) {
                status_index = r;
                break;
            }
        }
        if (status_index >= 0) {
            snprintf(root_path, sizeof(root_path), "%d", status_index);
            menu_web_append_status_items(buf, status_root, root_path, current, version, &first);
        }
    }
    menu_web_buffer_append(buf, "]");

    theme_root = menu_web_current_root(ctrl, &theme_root_index);
    menu_web_buffer_append(buf, ",\"background\":");
    if (theme_root && theme_root_index >= 0) {
        snprintf(root_path, sizeof(root_path), "%d", theme_root_index);
        menu_web_append_background_url(buf, theme_root, root_path, version);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
    menu_web_buffer_append(buf, ",\"font\":");
    if (theme_root && theme_root_index >= 0) {
        menu_web_append_menu_font_url(buf, theme_root, root_path, version);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
    menu_web_buffer_appendf(buf,
                            ",\"font_size\":%d",
                            theme_root ? menu_get_effective_font_size(theme_root) : 0);
    menu_web_buffer_append(buf, ",\"colors\":{\"background\":");
    menu_web_append_color(buf, ctrl->background_color);
    menu_web_buffer_append(buf, ",\"scale\":");
    menu_web_append_color(buf, ctrl->scale_color);
    menu_web_buffer_append(buf, ",\"indicator\":");
    menu_web_append_color(buf, ctrl->indicator_color);
    menu_web_buffer_append(buf, ",\"default\":");
    menu_web_append_color(buf, ctrl->default_color);
    menu_web_buffer_append(buf, ",\"selected\":");
    menu_web_append_color(buf, ctrl->selected_color);
    menu_web_buffer_append(buf, ",\"activated\":");
    menu_web_append_color(buf, ctrl->activated_color);
    menu_web_buffer_append(buf, "}}\n");
}

static void menu_web_handle_state(struct mg_connection *c,
                                  struct mg_http_message *hm,
                                  menu_web *web) {
    menu_web_buffer buf = {0};
    char view_path[128] = "";

    menu_web_query_value(hm->query, "path", view_path, sizeof(view_path));
    menu_web_append_state_json(&buf, web, view_path);
    if (!buf.data) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"out of memory\"}\n");
        return;
    }

    mg_http_reply(c,
                  200,
                  "Content-Type: application/json\r\nCache-Control: no-store\r\n",
                  "%s",
                  buf.data);
    menu_web_buffer_free(&buf);
}

static void menu_web_append_item(menu_web_buffer *buf,
                                 menu_item *item,
                                 const char *path,
                                 menu *current,
                                 unsigned long long revision,
                                 int *first) {
    menu *m = menu_item_get_menu(item);
    int id = menu_item_get_id(item);
    menu *sub_menu = menu_item_get_sub_menu(item);

    if (!*first) {
        menu_web_buffer_append(buf, ",");
    }
    *first = 0;

    menu_web_buffer_append(buf, "{");
    menu_web_buffer_append(buf, "\"path\":");
    menu_web_buffer_append_json_string(buf, path);
    menu_web_buffer_append(buf, ",\"label\":");
    menu_web_buffer_append_json_string(buf, menu_item_get_label(item));
    menu_web_buffer_append(buf, ",\"icon\":");
    menu_web_append_icon_url(buf, item, path);
    menu_web_buffer_append(buf, ",\"font\":");
    menu_web_append_font_url(buf, item, path, revision);
    menu_web_buffer_appendf(buf,
                            ",\"font_size\":%d,\"object_type\":%d,\"submenu\":%s,\"current\":%s,\"active\":%s",
                            menu_item_get_effective_font_size(item),
                            menu_item_get_object_type(item),
                            sub_menu ? "true" : "false",
                            m == current && menu_get_current_id(m) == id ? "true" : "false",
                            menu_get_active_id(m) == id ? "true" : "false");
    menu_web_buffer_append(buf, ",\"color\":");
    menu_web_append_color(buf, menu_web_item_color(item, current));

    if (sub_menu) {
        menu_web_buffer_append(buf, ",\"children\":");
        menu_web_append_menu_items(buf, sub_menu, path, current, revision);
    } else {
        menu_web_buffer_append(buf, ",\"children\":[]");
    }

    menu_web_buffer_append(buf, "}");
}

static char *menu_web_build_tree_json(menu_ctrl *ctrl) {
    menu_web_buffer buf = {0};
    menu *current = menu_ctrl_get_current(ctrl);
    menu *current_transient = menu_ctrl_get_current_transient(ctrl);
    unsigned int version = menu_web_style_version(ctrl);
    unsigned long long structure_hash = menu_web_structure_hash(ctrl);

    if (!menu_web_buffer_appendf(&buf, "{\"style_version\":%u,\"structure_version\":\"%016llx\",\"roots\":[", version, structure_hash)) {
        menu_web_buffer_free(&buf);
        return NULL;
    }

    for (int r = 0; r < menu_ctrl_get_root_count(ctrl); r++) {
        menu *root = menu_ctrl_get_root_at(ctrl, r);
        char path[32];
        snprintf(path, sizeof(path), "%d", r);
        if (r > 0) {
            menu_web_buffer_append(&buf, ",");
        }

        menu_web_buffer_append(&buf, "{");
        menu_web_buffer_append(&buf, "\"path\":");
        menu_web_buffer_append_json_string(&buf, path);
        menu_web_buffer_append(&buf, ",\"label\":");
        menu_web_buffer_append_json_string(&buf, menu_get_label(root));
        menu_web_buffer_append(&buf, ",\"font\":");
        menu_web_append_menu_font_url(&buf, root, path, version);
        menu_web_buffer_append(&buf, ",\"background\":");
        menu_web_append_background_url(&buf, root, path, version);
        menu_web_buffer_appendf(&buf,
                                ",\"font_size\":%d",
                                menu_get_effective_font_size(root));
        menu_web_buffer_appendf(&buf,
                                ",\"current\":%s,\"current_transient\":%s,\"active\":%s,\"transient\":%s,\"color\":",
                                root == current ? "true" : "false",
                                root == current_transient ? "true" : "false",
                                root == menu_ctrl_get_active(ctrl) ? "true" : "false",
                                menu_is_transient(root) ? "true" : "false");
        menu_web_append_color(&buf, menu_get_effective_default_color(root));
        menu_web_buffer_append(&buf, ",\"children\":");
        menu_web_append_menu_items(&buf, root, path, current, version);
        menu_web_buffer_append(&buf, "}");
    }
    menu_web_buffer_append(&buf, "]}\n");

    return buf.data;
}

static int menu_web_parse_index(const char **path, int *index) {
    const char *p = *path;
    int value = 0;

    if (!p || *p < '0' || *p > '9') {
        return 0;
    }

    while (*p >= '0' && *p <= '9') {
        value = 10 * value + (*p - '0');
        p++;
    }

    if (*p == '/') {
        p++;
    } else if (*p != '\0') {
        return 0;
    }

    *index = value;
    *path = p;
    return 1;
}

static int menu_web_resolve_path(menu_ctrl *ctrl,
                                 const char *path,
                                 menu **menu_out,
                                 menu_item **item_out) {
    int index;
    menu *m;
    menu_item *item = NULL;

    if (!menu_web_parse_index(&path, &index)) {
        return 0;
    }

    m = menu_ctrl_get_root_at(ctrl, index);
    if (!m) {
        return 0;
    }

    while (path && *path) {
        if (!menu_web_parse_index(&path, &index)) {
            return 0;
        }
        if (index < 0 || index > menu_get_max_id(m)) {
            return 0;
        }

        item = menu_get_item(m, index);
        if (!item) {
            return 0;
        }

        if (*path) {
            m = menu_item_get_sub_menu(item);
            if (!m) {
                return 0;
            }
        }
    }

    *menu_out = m;
    *item_out = item;
    return 1;
}

static int menu_web_query_value(struct mg_str query,
                                const char *name,
                                char *buffer,
                                size_t buffer_len) {
    size_t name_len = strlen(name);
    const char *p = query.buf;
    const char *end = query.buf + query.len;

    while (p < end) {
        const char *key = p;
        const char *eq = memchr(p, '=', (size_t) (end - p));
        const char *amp = memchr(p, '&', (size_t) (end - p));
        if (!amp) {
            amp = end;
        }
        if (eq && eq < amp && (size_t) (eq - key) == name_len && !strncmp(key, name, name_len)) {
            size_t len = (size_t) (amp - eq - 1);
            if (len >= buffer_len) {
                len = buffer_len - 1;
            }
            memcpy(buffer, eq + 1, len);
            buffer[len] = '\0';
            return 1;
        }
        p = amp + (amp < end ? 1 : 0);
    }

    return 0;
}

static void menu_web_handle_tree(struct mg_connection *c, menu_web *web) {
    char *json = menu_web_build_tree_json(web->ctrl);
    if (!json) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"out of memory\"}\n");
        return;
    }
    mg_http_reply(c,
                  200,
                  "Content-Type: application/json\r\nCache-Control: no-store\r\n",
                  "%s",
                  json);
    free(json);
}

static void menu_web_handle_background(struct mg_connection *c,
                                       struct mg_http_message *hm,
                                       menu_web *web) {
    char path[128];
    menu *m = NULL;
    menu_item *item = NULL;
    const char *background;
    struct mg_http_serve_opts opts = {
        .mime_types = "svg=image/svg+xml,png=image/png,jpg=image/jpeg,jpeg=image/jpeg,gif=image/gif,webp=image/webp,bmp=image/bmp"
    };

    if (!menu_web_query_value(hm->query, "path", path, sizeof(path))) {
        mg_http_reply(c, 400, "Content-Type: text/plain\r\n", "missing path\n");
        return;
    }

    if (!menu_web_resolve_path(web->ctrl, path, &m, &item)) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "background not found\n");
        return;
    }

    background = menu_get_effective_background_path(m);
    if (!background || !background[0]) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "background not found\n");
        return;
    }

    mg_http_serve_file(c, hm, background, &opts);
}

static void menu_web_handle_icon(struct mg_connection *c,
                                 struct mg_http_message *hm,
                                 menu_web *web) {
    char path[128];
    menu *m = NULL;
    menu_item *item = NULL;
    const char *icon;
    struct mg_http_serve_opts opts = {
        .mime_types = "svg=image/svg+xml,png=image/png,jpg=image/jpeg,jpeg=image/jpeg,gif=image/gif,webp=image/webp"
    };

    if (!menu_web_query_value(hm->query, "path", path, sizeof(path))) {
        mg_http_reply(c, 400, "Content-Type: text/plain\r\n", "missing path\n");
        return;
    }

    if (!menu_web_resolve_path(web->ctrl, path, &m, &item) || !item) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "icon not found\n");
        return;
    }

    icon = menu_item_get_icon(item);
    if (!icon || !icon[0]) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "icon not found\n");
        return;
    }

    mg_http_serve_file(c, hm, icon, &opts);
}

static void menu_web_handle_font(struct mg_connection *c,
                                 struct mg_http_message *hm,
                                 menu_web *web) {
    char path[128];
    menu *m = NULL;
    menu_item *item = NULL;
    const char *font;
    struct mg_http_serve_opts opts = {
        .mime_types = "ttf=font/ttf,otf=font/otf,woff=font/woff,woff2=font/woff2"
    };

    if (!menu_web_query_value(hm->query, "path", path, sizeof(path))) {
        mg_http_reply(c, 400, "Content-Type: text/plain\r\n", "missing path\n");
        return;
    }

    if (!menu_web_resolve_path(web->ctrl, path, &m, &item)) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "font not found\n");
        return;
    }

    font = item ? menu_item_get_effective_font_path(item) : menu_get_effective_font_path(m);
    if (!font || !font[0]) {
        mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "font not found\n");
        return;
    }

    mg_http_serve_file(c, hm, font, &opts);
}

static void menu_web_handle_activate(struct mg_connection *c,
                                     struct mg_http_message *hm,
                                     menu_web *web) {
    char path[128];
    menu *m = NULL;
    menu_item *item = NULL;

    if (!menu_web_query_value(hm->query, "path", path, sizeof(path))) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"missing path\"}\n");
        return;
    }

    if (!menu_web_resolve_path(web->ctrl, path, &m, &item)) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"path not found\"}\n");
        return;
    }

    if (item) {
        menu_item_warp_to(item);
        menu_ctrl_dispatch_item_event(web->ctrl, item, ACTIVATE);
    } else {
        menu_open(m);
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"ok\":true}\n");
}

static void menu_web_handle_event(struct mg_connection *c,
                                  struct mg_http_message *hm,
                                  menu_web *web) {
    char type[64];
    menu_event evt;

    if (!menu_web_query_value(hm->query, "type", type, sizeof(type))) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"missing type\"}\n");
        return;
    }

    if (!strcmp(type, "volume_up")) {
        evt = TURN_RIGHT_1;
    } else if (!strcmp(type, "volume_down")) {
        evt = TURN_LEFT_1;
    } else {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"unknown event\"}\n");
        return;
    }

    if (!menu_ctrl_dispatch_event(web->ctrl, evt)) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"dispatch failed\"}\n");
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"ok\":true}\n");
}

static const char *menu_web_index_html =
#include "index_html.inc"
;

static void menu_web_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        menu_web *web = (menu_web *) c->fn_data;

        if (menu_web_uri_eq(hm->uri, "/")) {
            mg_http_reply(c, 200, "Content-Type: text/html; charset=utf-8\r\n", "%s", menu_web_index_html);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/tree")) {
            menu_web_handle_tree(c, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/state")) {
            menu_web_handle_state(c, hm, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/icon")) {
            menu_web_handle_icon(c, hm, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/background")) {
            menu_web_handle_background(c, hm, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/font")) {
            menu_web_handle_font(c, hm, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/activate")) {
            menu_web_handle_activate(c, hm, web);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/event")) {
            menu_web_handle_event(c, hm, web);
        } else {
            mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "not found\n");
        }
    }
}

menu_web *menu_web_new(menu_ctrl *ctrl) {
    char *listen_url;

    if (!ctrl || !get_config_value_int("menu_web_enabled", 1)) {
        return NULL;
    }

    listen_url = get_config_value("menu_web_listen", MENU_WEB_DEFAULT_LISTEN);
    if (!listen_url || !listen_url[0]) {
        free(listen_url);
        return NULL;
    }

    menu_web *web = calloc(1, sizeof(menu_web));
    if (!web) {
        free(listen_url);
        return NULL;
    }

    web->ctrl = ctrl;
    web->listen_url = listen_url;
    menu_web_configure_mongoose_logging();
    mg_mgr_init(&web->mgr);
    if (!mg_http_listen(&web->mgr, web->listen_url, menu_web_handler, web)) {
        log_error(MENU_CTX, "Could not start menu web service on %s\n", web->listen_url);
        menu_web_free(web);
        return NULL;
    }

    log_info(MENU_CTX, "Menu web service listening on %s\n", web->listen_url);
    return web;
}

void menu_web_poll(menu_web *web, int timeout_ms) {
    if (web) {
        mg_mgr_poll(&web->mgr, timeout_ms);
    }
}

void menu_web_free(menu_web *web) {
    if (web) {
        mg_mgr_free(&web->mgr);
        free(web->listen_url);
        free(web);
    }
}

#else

menu_web *menu_web_new(menu_ctrl *ctrl) {
    (void) ctrl;
    return NULL;
}

void menu_web_poll(menu_web *web, int timeout_ms) {
    (void) web;
    (void) timeout_ms;
}

void menu_web_free(menu_web *web) {
    (void) web;
}

#endif
