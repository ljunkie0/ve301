/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE

#include "base.h"
#include "base/config.h"
#include "base/log_contexts.h"
#include "base/logging.h"
#include "base/util.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define INTERNET_CHECK_HOST "www.google.com"

static pthread_t __internet_thread = 0;
static int __internet_thread_run = 0;
static int __internet_available = 0;

char *user_home;
static char *__app;

char *my_copystr(const char *str) {
    if (str) {
        return strdup(str);
    }
    return NULL;
}

void time_check_interval_free(time_check_interval *i) {
    free(i);
}

time_check_interval *time_check_interval_new(int check_seconds) {
    time_check_interval *i = malloc(sizeof(time_check_interval));
    i->last_checked = 0;
    i->check_seconds = check_seconds;
    return i;
}

int check_time_interval(time_check_interval *i) {
    time_t timer;
    time(&timer);
    if (timer - i->last_checked > i->check_seconds) {
        i->last_checked = timer;
        return 1;
    }
    return 0;
}

network_interfaces *get_network_interfaces() {
    struct ifaddrs *ptr_ifaddrs = NULL;
    int r = getifaddrs(&ptr_ifaddrs);

    if (r) {
        return 0;
    }

    network_interfaces *interfaces = malloc(sizeof(network_interfaces));
    interfaces->n = 0;
    interfaces->interfaces = NULL;

    for (struct ifaddrs *ptr_entry = ptr_ifaddrs; ptr_entry; ptr_entry = ptr_entry->ifa_next) {
        sa_family_t address_family = ptr_entry->ifa_addr->sa_family;
        if (address_family == AF_INET) {
            interfaces->n++;
            interfaces->interfaces = realloc(interfaces->interfaces,
                                             interfaces->n * sizeof(network_interface *));
            network_interface *interface = malloc(sizeof(network_interface));
            interfaces->interfaces[interfaces->n - 1] = interface;

            interface->ifname = my_copystr(ptr_entry->ifa_name);
            if (ptr_entry->ifa_addr) {
                char buffer[INET_ADDRSTRLEN] = {
                    0,
                };
                inet_ntop(address_family,
                          &((struct sockaddr_in *) (ptr_entry->ifa_addr))->sin_addr,
                          buffer,
                          INET_ADDRSTRLEN);
                interface->ipaddress = my_copystr(buffer);

                log_info(BASE_CTX, "%s: %s\n", interface->ifname, interface->ipaddress);
            }
        }
    }

    freeifaddrs(ptr_ifaddrs);

    return interfaces;
}

void free_network_interface(network_interface *interface) {
    if (interface) {
        free_and_set_null((void **) &interface->ifname);
        free_and_set_null((void **) &interface->ipaddress);
        free_and_set_null((void **) &interface);
    }
}

void free_network_interfaces(network_interfaces *interfaces) {
    for (int n = 0; n < interfaces->n; n++) {
        free_network_interface(interfaces->interfaces[n]);
    }
    free(interfaces);
}

int __check_internet() {
    log_debug(BASE_CTX, "Checking internet...\n");
    const char *hostname = INTERNET_CHECK_HOST;
    log_debug(BASE_CTX, "1\n");
    struct addrinfo *result;
    log_debug(BASE_CTX, "2\n");
    int s = getaddrinfo(hostname, 0, 0, &result);
    log_debug(BASE_CTX, "Reply: %d\n", s);

    int reply = 0;
    if (s) {
        log_error(BASE_CTX, "Error checking internet: %s\n", gai_strerror(s));
    } else {
        freeaddrinfo(result);
        reply = 1;
    }

    log_config(BASE_CTX, "Internet available: %d\n", reply);

    return reply;
}

void *__internet_thread_start(void *arg) {
    log_info(BASE_CTX, "Internet thread successfully started\n");
    pthread_setname_np(pthread_self(), "Internet thread");

    const struct timespec duration = {0, 50000000};

    time_t timer;
    time(&timer);

    while (__internet_thread_run) {
        nanosleep(&duration, NULL);

        time_t timer2;
        time(&timer2);

        time_t diff = timer2 - timer;

        if (diff > 3) {
            timer = timer2;
            __internet_available = __check_internet();
        }
    }

    log_info(BASE_CTX, "Internet thread finished\n");
    return NULL;
}

void __start_internet_thread() {
    __internet_thread_run = 1;
    int r = pthread_create(&__internet_thread, NULL, __internet_thread_start, NULL);
    if (r) {
        __internet_thread_run = 0;
        log_error(BASE_CTX, "Could not start internet thread: %d\n", r);
    }
}

void __stop_internet_thread() {
    if (__internet_thread_run) {
        __internet_thread_run = 0;
        void *res;
        pthread_join(__internet_thread, &res);
    }
}

int check_internet() {
    if (!__internet_thread_run) {
        __internet_available = __check_internet();
        __start_internet_thread();
    }
    return __internet_available;
}

void base_init(const char *appname, FILE *dflt_log_file, int log_level) {
    __app = my_copystr(appname);
    init_log_file(appname, dflt_log_file);

    init_config_file(appname);
    char *lls = get_config_value("log_levels", "11111111");
    int i = -1;
    while (lls[++i]) {
        char c[2];
        c[0] = lls[i];
        c[1] = 0;
        set_log_level(i, atoi(c));
    }
    free(lls);

    while (i < NUM_CTX) {
        set_log_level(i++, 1);
    }

    log_info(BASE_CTX, "Log levels:\n");
    log_info(BASE_CTX, "\t%d", get_log_level(0));

    for (i = 1; i < NUM_CTX; i++) {
        log_info(BASE_CTX, " %d", get_log_level(i));
    }
    log_info(BASE_CTX, "\n");
    log_info(BASE_CTX, "Base init done\n");
}

void base_close() {
    __stop_internet_thread();
    free_config();
    close_log_file();
}
