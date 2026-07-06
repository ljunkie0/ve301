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

#include "../base/config.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "menu_item.h"
#include "menu_menu.h"
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
        log_trace(MENU_CTX, "mongoose: %s\n", line);
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
                                 int *first);

static void menu_web_append_menu_items(menu_web_buffer *buf,
                                       menu *m,
                                       const char *path,
                                       menu *current) {
    int first = 1;
    menu_web_buffer_append(buf, "[");
    for (int i = 0; i <= menu_get_max_id(m); i++) {
        menu_item *item = menu_get_item(m, i);
        if (!item || !menu_item_get_visible(item)) {
            continue;
        }

        char child_path[256];
        snprintf(child_path, sizeof(child_path), "%s/%d", path, i);
        menu_web_append_item(buf, item, child_path, current, &first);
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

static void menu_web_append_font_path_url(menu_web_buffer *buf, const char *font_path, const char *path) {
    char font_url[320];

    if (font_path && font_path[0]) {
        snprintf(font_url, sizeof(font_url), "/api/menu/font?path=%s", path);
        menu_web_buffer_append_json_string(buf, font_url);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
}

static void menu_web_append_font_url(menu_web_buffer *buf, menu_item *item, const char *path) {
    menu_web_append_font_path_url(buf, menu_item_get_effective_font_path(item), path);
}

static void menu_web_append_menu_font_url(menu_web_buffer *buf, menu *m, const char *path) {
    menu_web_append_font_path_url(buf, menu_get_effective_font_path(m), path);
}

static void menu_web_append_background_url(menu_web_buffer *buf, menu *m, const char *path) {
    const char *bg_path = menu_get_effective_background_path(m);
    char bg_url[320];

    if (bg_path && bg_path[0]) {
        snprintf(bg_url, sizeof(bg_url), "/api/menu/background?path=%s", path);
        menu_web_buffer_append_json_string(buf, bg_url);
    } else {
        menu_web_buffer_append_json_string(buf, NULL);
    }
}

static void menu_web_append_item(menu_web_buffer *buf,
                                 menu_item *item,
                                 const char *path,
                                 menu *current,
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
    menu_web_append_font_url(buf, item, path);
    menu_web_buffer_appendf(buf,
                            ",\"font_size\":%d,\"object_type\":%d,\"submenu\":%s,\"current\":%s,\"active\":%s",
                            menu_item_get_effective_font_size(item),
                            menu_item_get_object_type(item),
                            sub_menu ? "true" : "false",
                            m == current && menu_get_current_id(m) == id ? "true" : "false",
                            menu_get_active_id(m) == id ? "true" : "false");

    if (sub_menu) {
        menu_web_buffer_append(buf, ",\"children\":");
        menu_web_append_menu_items(buf, sub_menu, path, current);
    } else {
        menu_web_buffer_append(buf, ",\"children\":[]");
    }

    menu_web_buffer_append(buf, "}");
}

static char *menu_web_build_tree_json(menu_ctrl *ctrl) {
    menu_web_buffer buf = {0};
    menu *current = menu_ctrl_get_current(ctrl);
    menu *current_transient = menu_ctrl_get_current_transient(ctrl);

    if (!menu_web_buffer_append(&buf, "{\"roots\":[")) {
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
        menu_web_append_menu_font_url(&buf, root, path);
        menu_web_buffer_append(&buf, ",\"background\":");
        menu_web_append_background_url(&buf, root, path);
        menu_web_buffer_appendf(&buf,
                                ",\"font_size\":%d",
                                menu_get_effective_font_size(root));
        menu_web_buffer_appendf(&buf,
                                ",\"current\":%s,\"current_transient\":%s,\"active\":%s,\"transient\":%s,\"children\":",
                                root == current ? "true" : "false",
                                root == current_transient ? "true" : "false",
                                root == menu_ctrl_get_active(ctrl) ? "true" : "false",
                                menu_is_transient(root) ? "true" : "false");
        menu_web_append_menu_items(&buf, root, path, current);
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
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
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
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>VE301 Menu</title>"
    "<style>"
    "body{margin:0;font:16px system-ui,sans-serif;background:#101010;color:#f2f2f2;padding-bottom:96px}"
    "#app{display:grid;grid-template-columns:minmax(220px,32vw) 1fr;min-height:calc(100vh - 96px)}"
    "nav{border-right:1px solid #333;padding:16px;overflow:auto;height:calc(100vh - 96px);box-sizing:border-box}"
    "#content{position:fixed;left:0;right:0;bottom:0;min-height:72px;max-height:30vh;padding:10px 16px;display:flex;flex-wrap:wrap;gap:8px 18px;align-items:center;background:rgba(16,16,16,.92);border-top:1px solid #333;box-sizing:border-box;overflow:auto;z-index:9}"
    "#controls{position:fixed;right:16px;top:16px;display:flex;gap:8px;z-index:10}"
    "#controls button{width:44px;height:44px;border-radius:22px;text-align:center;font-size:22px;padding:0}"
    "button{font:inherit;color:inherit;background:#202020;border:1px solid #444;padding:8px 10px;border-radius:6px;text-align:left;cursor:pointer}"
    "button:hover{background:#2d2d2d}.node{display:grid;gap:6px;margin:4px 0 4px 16px}.root{margin-left:0}"
    ".active{border-color:#e0c45c;color:#ffe78a}.status-item{display:flex;align-items:center;gap:8px;min-width:0;max-width:100%;padding:4px 0}"
    ".label{font-size:22px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:min(60vw,520px)}.weather{font-family:'ve301-weather';font-size:48px;line-height:1}.icon{width:32px;height:32px;object-fit:contain;display:block;flex:0 0 auto}.meta{color:#aaa;font-size:13px}"
    "@media(max-width:700px){body{padding-bottom:128px}#app{grid-template-columns:1fr;min-height:calc(100vh - 128px)}nav{border-right:0;border-bottom:1px solid #333;height:calc(100vh - 128px);padding-top:72px}#content{min-height:104px}.label{max-width:92vw}}"
    "</style></head><body><div id=\"app\"><nav id=\"tree\"></nav><main id=\"content\"></main></div><div id=\"controls\"><button type=\"button\" id=\"vol-down\" title=\"Volume down\">-</button><button type=\"button\" id=\"vol-up\" title=\"Volume up\">+</button></div>"
    "<script>"
    "const open=new Set();"
    "const loadedFonts=new Set();"
    "function fontFamily(url){let h=0;for(let i=0;i<url.length;i++)h=(h*31+url.charCodeAt(i))>>>0;return 've301-'+h}"
    "function ensureFont(url){if(!url)return '';let f=fontFamily(url);if(loadedFonts.has(url))return f;loadedFonts.add(url);let s=document.createElement('style');s.textContent='@font-face{font-family:'+f+';src:url('+url+') format(truetype);font-display:block;}';document.head.appendChild(s);return f}"
    "async function post(u){await fetch(u,{method:'POST'});load()}"
    "async function sendEvent(type){await post('/api/menu/event?type='+type)}"
    "function node(n,root){let d=document.createElement('div');d.className='node '+(root?'root':'');"
    "let children=n.children||[];let hasChildren=n.submenu||children.length>0;let isOpen=open.has(n.path);"
    "let b=document.createElement('button');b.textContent=(hasChildren?(isOpen?'- ':'+ '):'')+(n.label||'(leer)');if(n.font)b.style.fontFamily=ensureFont(n.font);if(n.active||n.current)b.className='active';"
    "b.onclick=()=>{if(hasChildren){isOpen?open.delete(n.path):open.add(n.path)}post('/api/menu/activate?path='+n.path)};d.appendChild(b);"
    "if(isOpen)children.forEach(c=>d.appendChild(node(c,false)));return d}"
    "function flatten(n,a){(n.children||[]).forEach(c=>{a.push(c);flatten(c,a)})}"
    "async function load(){let r=await fetch('/api/menu/tree');let data=await r.json();"
    "let roots=data.roots||[];let bg=(roots.find(x=>x.current_transient&&x.background)||roots.find(x=>!x.transient&&x.background)||roots.find(x=>x.background)||{}).background;document.body.style.backgroundImage=bg?'url('+bg+')':'';document.body.style.backgroundSize='cover';document.body.style.backgroundPosition='center';let t=document.getElementById('tree');t.textContent='';roots.filter(x=>!x.transient).forEach(x=>t.appendChild(node(x,true)));"
    "let display=roots.find(x=>x.current_transient);let items=[];if(display)flatten(display,items);"
    "let c=document.getElementById('content');c.textContent='';items.forEach(i=>{let e=document.createElement('section');e.className='status-item';"
    "e.innerHTML=i.icon?'<img class=\"icon\" src=\"\"><div class=\"label\"></div>':'<div class=\"label\"></div>';"
    "let label=e.querySelector('.label');if(i.font)label.style.fontFamily=ensureFont(i.font);if(i.font_size)label.style.fontSize=Math.min(i.font_size,42)+'px';"
    "if(i.icon)e.querySelector('img').src=i.icon;label.textContent=i.label||'';c.appendChild(e)});}"
    "document.getElementById('vol-up').onclick=()=>sendEvent('volume_up');document.getElementById('vol-down').onclick=()=>sendEvent('volume_down');"
    "document.addEventListener('keydown',e=>{if(e.repeat)return;let k=e.key;if(k==='AudioVolumeUp'||k==='VolumeUp'){e.preventDefault();sendEvent('volume_up')}else if(k==='AudioVolumeDown'||k==='VolumeDown'){e.preventDefault();sendEvent('volume_down')}});"
    "load();setInterval(load,2000);"
    "</script></body></html>";

static void menu_web_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        menu_web *web = (menu_web *) c->fn_data;

        if (menu_web_uri_eq(hm->uri, "/")) {
            mg_http_reply(c, 200, "Content-Type: text/html; charset=utf-8\r\n", "%s", menu_web_index_html);
        } else if (menu_web_uri_eq(hm->uri, "/api/menu/tree")) {
            menu_web_handle_tree(c, web);
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
