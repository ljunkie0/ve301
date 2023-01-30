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
#include "base.h"
#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MATCHING_RULE "eavesdrop=true,interface='org.bluealsa.Manager1'"
#define MATCHING_RULE1 "eavesdrop=true,interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'"

const char *pcm_removed = "PCMRemoved";
const char *pcm_added = "PCMAdded";
const char *prop_changed = "PropertiesChanged";
const char *prop_bluez_device = "org.bluez.Device1";
const char *track_key = "Track";
const char *title_key = "Title";
const char *artist_key = "Artist";
const char *album_key = "Album";

DBusMessage* bt_msg;
DBusConnection* bt_conn;
DBusError bt_err;

typedef struct bt_info_t {
    int connected;
    char *title;
    char *artist;
    char *album;
} bt_info_t;

static bt_info_t *bt_info;

int bt_is_connected() {
   return bt_info->connected;
}

char *bt_get_title() {
    return bt_info->title;
}

char *bt_get_album() {
    return bt_info->album;
}

char *bt_get_artist() {
    return bt_info->artist;
}

void bt_init() {
    bt_info = malloc(sizeof(bt_info_t));
    bt_info->connected = 0;
    bt_info->title = NULL;
    bt_info->album = NULL;
    bt_info->artist = NULL;

    // initialise the error
    dbus_error_init(&bt_err);

    // connect to the bus and check for errors
    bt_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &bt_err);
    if (dbus_error_is_set(&bt_err)) {
       log_error(BT_CTX, "Connection Error (%s)\n", bt_err.message);
       dbus_error_free(&bt_err);
    }

    if (NULL == bt_conn) {
       log_error(BT_CTX, "Connection Null\n");
    } else {
        dbus_bus_add_match (bt_conn,MATCHING_RULE,&bt_err);
        if (dbus_error_is_set(&bt_err)) {
           log_error(BT_CTX, "Connection Error (%s)\n", bt_err.message);
           dbus_error_free(&bt_err);
        }
        dbus_bus_add_match (bt_conn,MATCHING_RULE1,&bt_err);
        if (dbus_error_is_set(&bt_err)) {
            log_error(BT_CTX, "Connection Error (%s)\n", bt_err.message);
            dbus_error_free(&bt_err);
        }
    }

}

void bt_close() {
    // close the connection
    if (bt_conn != NULL) {
        dbus_connection_close(bt_conn);
    }
}

void process_string_dict_entry(DBusMessageIter parentIter,char **key, char **value) {
    DBusMessageIter dictEntryIter;
    dbus_message_iter_recurse(&parentIter,&dictEntryIter);
    dbus_message_iter_get_basic(&dictEntryIter,key);
    dbus_message_iter_next(&dictEntryIter);
    DBusMessageIter variantIter;
    dbus_message_iter_recurse(&dictEntryIter,&variantIter);
    if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(&variantIter,value);
        return;
    }

    *value = NULL;

}

void process_properties_chg_msg(DBusMessage *msg) {
    DBusMessageIter iter;
    dbus_message_iter_init(msg,&iter);
    char *str;
    dbus_message_iter_get_basic(&iter,&str);

    if (dbus_message_iter_has_next(&iter)) {
        dbus_message_iter_next (&iter);
        DBusMessageIter subIter;
        dbus_message_iter_recurse(&iter,&subIter);
        while (dbus_message_iter_get_arg_type(&subIter) == DBUS_TYPE_DICT_ENTRY) {

            DBusMessageIter dictEntryIter;
            dbus_message_iter_recurse(&subIter,&dictEntryIter);
            char *key;
            dbus_message_iter_get_basic(&dictEntryIter,&key);

            if (dbus_message_iter_has_next(&dictEntryIter)) {
                dbus_message_iter_next (&dictEntryIter);
                if (!strcmp(track_key,key)) {
                    DBusMessageIter dictValueIter;
                    dbus_message_iter_recurse(&dictEntryIter,&dictValueIter);
                    DBusMessageIter trackDataIter;
                    dbus_message_iter_recurse(&dictValueIter,&trackDataIter);
                    char current_type = dbus_message_iter_get_arg_type(&trackDataIter);
                    while (current_type == DBUS_TYPE_DICT_ENTRY) {
                        char *key, *value;
                        process_string_dict_entry(trackDataIter,&key,&value);
                        if (key) {
                            if (value && strlen(value) == 0) {
                                value = NULL;
                            }
                            if (strcmp(key,album_key)) {
                                bt_info->album = value;
                            } else if (strcmp(key,artist_key)) {
                                bt_info->artist = value;
                            } else if (strcmp(key,title_key)) {
                                bt_info->title = value;
                            }
                        }
                        dbus_message_iter_next (&trackDataIter);
                        current_type = dbus_message_iter_get_arg_type(&trackDataIter);
                    }
                }
            }

            dbus_message_iter_next (&subIter);
        }
    }

}

void debug_bt_info() {
    log_info(BT_CTX, "Connected: %d\n", bt_info->connected);
    log_info(BT_CTX, "Track:\n");
    log_info(BT_CTX, "\tTitle:  %s\n", bt_info->title);
    log_info(BT_CTX, "\tArtist: %s\n", bt_info->artist);
    log_info(BT_CTX, "\tAlbum:  %s\n", bt_info->album);
}

/**
 * 1 bluetooth device connected
 * 0 no change
 * -1 bluetooth device disconnected
 */
int bt_connection_signal(int proces_prop_changes) {

    int result = 0;

    if (bt_conn != NULL) {
        dbus_connection_read_write(bt_conn, 0);
        bt_msg = dbus_connection_pop_message(bt_conn);
        if (bt_msg != NULL) {

            const char *member = dbus_message_get_member(bt_msg);
            if (!strcmp(member,pcm_added)) {
                bt_info->connected = 1;
                result = 1;
            } else if (!strcmp(member,pcm_removed)) {
                bt_info->connected = 0;
                result = 1;
            } else if (proces_prop_changes && !strcmp(member,prop_changed)) {
                process_properties_chg_msg(bt_msg);
                result = 1;
            }

            dbus_message_unref(bt_msg);

        }
    }

    return result;

}
