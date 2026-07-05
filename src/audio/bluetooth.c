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

#define DBUS_API_SUBJECT_TO_CHANGE
#include "bluetooth.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "player_if.h"
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

struct __bluetooth_data {
    DBusMessage *message;
    DBusConnection *connection;
    DBusError error;
    player *player;
} __bluetooth_data;

void __bt_process_string_dict_entry(DBusMessageIter parentIter, char **key, char **value) {
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

void __bt_process_properties_chg_msg(DBusMessage *msg) {
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
                        player_set_active(__bluetooth_data.player, connected);
                    }
                } else if (!strcmp(track_key, key)) {
                    DBusMessageIter trackDataIter;
                    dbus_message_iter_recurse(&dictValueIter, &trackDataIter);
                    char current_type = dbus_message_iter_get_arg_type(&trackDataIter);
                    while (current_type == DBUS_TYPE_DICT_ENTRY) {
                        char *key, *value;
                        __bt_process_string_dict_entry(trackDataIter, &key, &value);
                        if (key) {
                            if (value && strlen(value) == 0) {
                                value = NULL;
                            }
                            if (!strcmp(key, album_key)) {
                                player_set_album(__bluetooth_data.player, value);
                            } else if (!strcmp(key, artist_key)) {
                                player_set_artist(__bluetooth_data.player, value);
                            } else if (!strcmp(key, title_key)) {
                                player_set_title(__bluetooth_data.player, value);
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

void __bt_log_info() {
    if (log_level_enabled(BT_CTX, IR_LOG_LEVEL_CONFIG)) {
        log_config(BT_CTX,
                   "Connected: %s\n",
                   player_is_active(__bluetooth_data.player) ? "yes" : "no");
        log_config(BT_CTX, "Track:\n");
        log_config(BT_CTX, "\tTitle:  %s\n", player_get_title(__bluetooth_data.player));
        log_config(BT_CTX, "\tArtist: %s\n", player_get_artist(__bluetooth_data.player));
        log_config(BT_CTX, "\tAlbum:  %s\n", player_get_album(__bluetooth_data.player));
    }
}

int __bt_connect() {
    // initialise the error
    dbus_error_init(&__bluetooth_data.error);

    // connect to the bus and check for errors
    __bluetooth_data.connection = dbus_bus_get(DBUS_BUS_SYSTEM, &__bluetooth_data.error);
    if (dbus_error_is_set(&__bluetooth_data.error)) {
        log_error(BT_CTX, "Error connecting to system bus: %s\n", __bluetooth_data.error.message);
        dbus_error_free(&__bluetooth_data.error);
    }

    if (__bluetooth_data.connection != NULL) {
        dbus_bus_add_match(__bluetooth_data.connection, MATCHING_RULE, &__bluetooth_data.error);
        if (dbus_error_is_set(&__bluetooth_data.error)) {
            log_config(BT_CTX,
                       "Error adding matching rule %s: %s\n",
                       MATCHING_RULE,
                       __bluetooth_data.error.message);
            dbus_error_free(&__bluetooth_data.error);
            dbus_connection_unref(__bluetooth_data.connection);
            __bluetooth_data.connection = NULL;
        }
    }

    if (__bluetooth_data.connection != NULL) {
        dbus_bus_add_match(__bluetooth_data.connection, MATCHING_RULE1, &__bluetooth_data.error);
        if (dbus_error_is_set(&__bluetooth_data.error)) {
            log_config(BT_CTX,
                       "Error adding matching rule %s: %s\n",
                       MATCHING_RULE1,
                       __bluetooth_data.error.message);
            dbus_error_free(&__bluetooth_data.error);
            dbus_connection_unref(__bluetooth_data.connection);
            __bluetooth_data.connection = NULL;
        }
    }
    return 1;
}

int __bt_init() {
    return __bt_connect();
}

int __bt_run() {
    if (__bluetooth_data.connection == NULL) {
        __bt_connect();
    }

    int process_prop_changes = 1;
    int result = 1;

    if (__bluetooth_data.connection != NULL) {
        dbus_connection_read_write(__bluetooth_data.connection, 0);
        __bluetooth_data.message = dbus_connection_pop_message(__bluetooth_data.connection);
        while (__bluetooth_data.message != NULL) {
            log_config(BT_CTX, "__bluetooth_data.message: %s\n", __bluetooth_data.message);
            const char *member = dbus_message_get_member(__bluetooth_data.message);
            log_config(BT_CTX, "member: %s\n", member);
            if (!strncmp(member, media_control, strlen(media_control))) {
                __bt_process_properties_chg_msg(__bluetooth_data.message);
                player_set_active(__bluetooth_data.player, 1);
                result = 1;
            } else if (!strcmp(member, pcm_removed)) {
                player_set_active(__bluetooth_data.player, 0);
                result = 1;
            } else if (!strcmp(member, prop_changed)) {
                __bt_process_properties_chg_msg(__bluetooth_data.message);
                result = 1;
            }

            dbus_message_unref(__bluetooth_data.message);
            __bluetooth_data.message = dbus_connection_pop_message(__bluetooth_data.connection);
        }
    } else {
        log_warning(BT_CTX, "No bluetooth connection\n");
        result = 1;
    }

    if (result) {
        log_config(BT_CTX,
                   "bt_run(proces_prop_changes => %d) -> %d\n",
                   process_prop_changes,
                   result);
        __bt_log_info();
    }

    return result;
}

int __bt_cleanup() {
    log_config(BT_CTX, "Closing ...\n");
    // close the connection
    if (__bluetooth_data.connection != NULL) {
        dbus_connection_unref(__bluetooth_data.connection);
        __bluetooth_data.connection = NULL;
    }
    player_free(__bluetooth_data.player);
    log_config(BT_CTX, "Closing done\n");
    return 1;
}

player *bluetooth_init(char *label, char *icon, int check_millis) {
    __bluetooth_data.player = player_new("BLUETOOTH",
                                         icon,
                                         label,
                                         check_millis,
                                         &__bt_init,
                                         &__bt_run,
                                         &__bt_cleanup,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);

    return __bluetooth_data.player;
}
