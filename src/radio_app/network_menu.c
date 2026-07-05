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

#include "private.h"

static int item_action_update_interface_menu(
    menu_event evt, menu *m, menu_item *item) {
    if (evt == DISPOSE) {
        if (menu_item_get_user_data(item)) {
            network_interface *interface = (network_interface *) menu_item_get_user_data(item);
            free_network_interface(interface);
            menu_item_set_user_data(item, NULL);
        }
    } else if (menu_item_get_sub_menu(item)) {
        menu *sub_menu = menu_item_get_sub_menu(item);

        menu_clear(sub_menu);

        network_interface *interface = (network_interface *) menu_item_get_user_data(item);
        char *ip_address_label = my_catstr("IP\n", interface->ipaddress);
        menu_item_new(
            sub_menu, ip_address_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
        free(ip_address_label);

        log_debug(MAIN_CTX, "Getting result from wifi scan\n");
        struct station_info *wifi_scan_result = scan_wifi_station(interface->ifname);
        log_debug(MAIN_CTX, "Result: %p\n", wifi_scan_result);

        if (wifi_scan_result) {
            char *ssid_label = my_catstr("Station\n", wifi_scan_result->ssid);
            log_debug(MAIN_CTX, "%s\n", ssid_label);
            menu_item_new(
                sub_menu, ssid_label, NULL, NULL, UNKNOWN_OBJECT_TYPE, NULL, -1, NULL, NULL, -1);
            free(ssid_label);

            char signal_chr[100];
            sprintf(signal_chr, "%d dBm", wifi_scan_result->signal_dbm);
            char *strength_label = my_catstr("Strength\n", signal_chr);
            menu_item_new(sub_menu,
                          strength_label,
                          NULL,
                          NULL,
                          UNKNOWN_OBJECT_TYPE,
                          NULL,
                          -1,
                          NULL,
                          NULL,
                          -1);
            free(strength_label);

            free(wifi_scan_result);
            log_debug(MAIN_CTX, "Done\n");
        }
    }

    return 0;
}

int item_action_update_network_menu(
    menu_event evt, menu *m, menu_item *item) {
    if (evt == DISPOSE) {
        return 0;
    }

    menu_item_free_user_data(item);

    menu *sub_menu = menu_item_get_sub_menu(item);
    if (sub_menu) {
        int id = menu_get_max_id(sub_menu);
        while (id >= 0) {
            menu_item_free_user_data(menu_get_item(sub_menu, id--));
        }
        menu_clear(sub_menu);
    }

    network_interfaces *interfaces = get_network_interfaces();

    if (interfaces) {
        menu_item_set_user_data(item, interfaces->interfaces);
        for (int i = 0; i < interfaces->n; i++) {
            network_interface *interface = interfaces->interfaces[i];
            interfaces->interfaces[i] = NULL;
            menu *interface_menu = menu_new(menu_get_ctrl(m), 1, NULL, 0, NULL, NULL, 0);
            menu_set_no_items_on_scale(interface_menu, 3);
            menu_item *interface_item = menu_add_sub_menu(menu_item_get_sub_menu(item),
                                                          interface->ifname,
                                                          interface_menu,
                                                          &item_action_update_interface_menu);
            menu_item_set_user_data(interface_item, interface);
        }

        free(interfaces->interfaces);
        free(interfaces);
    }

    return 0;
}
