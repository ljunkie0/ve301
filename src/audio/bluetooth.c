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

#define DBUS_API_SUBJECT_TO_CHANGE
#include "bluetooth.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MATCHING_RULE "eavesdrop=true,interface='org.bluealsa.Manager1'"
#define MATCHING_RULE1                                                         \
"eavesdrop=true,interface='org.freedesktop.DBus.Properties',member='"        \
    "PropertiesChanged'"

const char *pcm_removed = "PCMRemoved";
const char *pcm_added = "PCMAdded";
const char *media_control = "org.bluez.MediaControl";
const char *prop_changed = "PropertiesChanged";
const char *prop_bluez_device = "org.bluez.Device1";
const char *connected_key = "Connected";
const char *track_key = "Track";
const char *title_key = "Title";
const char *artist_key = "Artist";
const char *album_key = "Album";

DBusMessage *bt_msg;
DBusConnection *bt_conn;
DBusError bt_err;

player *__bt_player;

void process_string_dict_entry(DBusMessageIter parentIter, char **key,
                               char **value) {
    DBusMessageIter dictEntryIter;
    dbus_message_iter_recurse(&parentIter, &dictEntryIter);
    dbus_message_iter_get_basic(&dictEntryIter, key);
    dbus_message_iter_next(&dictEntryIter);
    DBusMessageIter variantIter;
    dbus_message_iter_recurse(&dictEntryIter, &variantIter);
    if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(&variantIter, value);
        log_config(BT_CTX, "process_string_dict_entry(%s,%s)\n", *key, *value);
        return;
    }

    *value = NULL;
}

void process_properties_chg_msg(DBusMessage *msg) {
    DBusMessageIter iter;
    dbus_message_iter_init(msg, &iter);
    char *str;
    dbus_message_iter_get_basic(&iter, &str);
    log_config(BT_CTX, "process_properties_chg_msg: str = %s\n", str);

    if (dbus_message_iter_has_next(&iter)) {
        dbus_message_iter_next(&iter);
        DBusMessageIter subIter;
        dbus_message_iter_recurse(&iter, &subIter);
        while (dbus_message_iter_get_arg_type(&subIter) == DBUS_TYPE_DICT_ENTRY) {

            DBusMessageIter dictEntryIter;
            dbus_message_iter_recurse(&subIter, &dictEntryIter);
            char *key;
            dbus_message_iter_get_basic(&dictEntryIter, &key);
            log_config(BT_CTX, "Key: %s\n", key);

            if (dbus_message_iter_has_next(&dictEntryIter)) {
                dbus_message_iter_next(&dictEntryIter);
                DBusMessageIter dictValueIter;
                dbus_message_iter_recurse(&dictEntryIter, &dictValueIter);
                if (!strcmp(connected_key, key)) {
                    DBusMessageIter variantIter;
                    dbus_message_iter_recurse(&dictEntryIter, &variantIter);
                    if (dbus_message_iter_get_arg_type(&variantIter) ==
                        DBUS_TYPE_BOOLEAN) {
                        dbus_bool_t connected;
                        dbus_message_iter_get_basic(&variantIter, &connected);
                        log_config(BT_CTX, "Connected: %d\n", connected);
                        player_set_status(__bt_player, connected);
                    }
                } else if (!strcmp(track_key, key)) {
                    DBusMessageIter trackDataIter;
                    dbus_message_iter_recurse(&dictValueIter, &trackDataIter);
                    char current_type = dbus_message_iter_get_arg_type(&trackDataIter);
                    while (current_type == DBUS_TYPE_DICT_ENTRY) {
                        char *key, *value;
                        process_string_dict_entry(trackDataIter, &key, &value);
                        if (key) {
                            if (value && strlen(value) == 0) {
                                value = NULL;
                            }
                            if (!strcmp(key, album_key)) {
                                player_set_album(__bt_player, value);
                            } else if (!strcmp(key, artist_key)) {
                                player_set_artist(__bt_player, value);
                            } else if (!strcmp(key, title_key)) {
                                player_set_title(__bt_player, value);
                            }
                        }
                        dbus_message_iter_next(&trackDataIter);
                        current_type = dbus_message_iter_get_arg_type(&trackDataIter);
                    }
                }
            } else {
                log_config(BT_CTX, "No further entry\n");
            }

            dbus_message_iter_next(&subIter);
        }
    }
}

void log_bt_info() {
    if (log_level_enabled(BT_CTX, IR_LOG_LEVEL_CONFIG)) {
        log_config(BT_CTX, "Connected: %s\n", player_get_status(__bt_player) ? "yes" : "no");
        log_config(BT_CTX, "Track:\n");
        log_config(BT_CTX, "\tTitle:  %s\n", player_get_title(__bt_player));
        log_config(BT_CTX, "\tArtist: %s\n", player_get_artist(__bt_player));
        log_config(BT_CTX, "\tAlbum:  %s\n", player_get_album(__bt_player));
    }
}

/**
 * 1 bluetooth device connected
 * 0 no change
 * -1 bluetooth device disconnected
 */
int bt_connection_signal() {

    int process_prop_changes = 1;
    int result = 0;

    if (bt_conn != NULL) {
        dbus_connection_read_write(bt_conn, 0);
        bt_msg = dbus_connection_pop_message(bt_conn);
        while (bt_msg != NULL) {
            log_config(BT_CTX, "bt_msg: %s\n", bt_msg);
            const char *member = dbus_message_get_member(bt_msg);
            log_config(BT_CTX, "member: %s\n", member);
            if (!strncmp(member, media_control, strlen(media_control))) {
                process_properties_chg_msg(bt_msg);
                player_set_status(__bt_player,1);
                result = 1;
            } else if (!strcmp(member, pcm_removed)) {
                player_set_status(__bt_player,0);
                result = -1;
            } else if (!strcmp(member, prop_changed)) {
                process_properties_chg_msg(bt_msg);
                result = 1;
            }

            dbus_message_unref(bt_msg);
            bt_msg = dbus_connection_pop_message(bt_conn);
        }
    } else {
        log_error(BT_CTX, "bt_connection_signal: no bluetooth connection\n");
    }

    if (result) {
        log_config(BT_CTX,
                   "bt_connection_signal(proces_prop_changes => %d) -> %d\n",
                   process_prop_changes, result);
        log_bt_info();
    }

    return result;
}

player *bt_init(char *label, char *icon, int check_seconds) {

    __bt_player = player_new("BLUETOOTH", icon, label, check_seconds, &bt_connection_signal);

    // initialise the error
    dbus_error_init(&bt_err);

    // connect to the bus and check for errors
    bt_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &bt_err);
    if (dbus_error_is_set(&bt_err)) {
        log_error(BT_CTX, "Error connecting to system bus: %s\n", bt_err.message);
        dbus_error_free(&bt_err);
    }

    if (NULL == bt_conn) {
        log_error(BT_CTX, "No connection available\n");
    } else {
        dbus_bus_add_match(bt_conn, MATCHING_RULE, &bt_err);
        if (dbus_error_is_set(&bt_err)) {
            log_error(BT_CTX, "Error adding matching rule %s: %s\n", MATCHING_RULE,
                      bt_err.message);
            dbus_error_free(&bt_err);
        }
        dbus_bus_add_match(bt_conn, MATCHING_RULE1, &bt_err);
        if (dbus_error_is_set(&bt_err)) {
            log_error(BT_CTX, "Error adding matching rule %s: %s\n", MATCHING_RULE1,
                      bt_err.message);
            dbus_error_free(&bt_err);
        }
    }

    return __bt_player;
}

void bt_close() {
    log_config(BT_CTX, "Closing ...\n");
    // close the connection
    if (bt_conn != NULL) {
        dbus_connection_unref(bt_conn);
        bt_conn = NULL;
    }
    player_free(__bt_player);
    log_config(BT_CTX, "Closing done\n");
}
