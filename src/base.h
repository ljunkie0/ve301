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

#ifndef BASE_H
#define BASE_H

#include<stdio.h>
#include<time.h>
#include<inttypes.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 5
#endif

#define IR_LOG_LEVEL_OFF 0
#define IR_LOG_LEVEL_ERROR 1
#define IR_LOG_LEVEL_WARNING 2
#define IR_LOG_LEVEL_INFO 3
#define IR_LOG_LEVEL_CONFIG 4
#define IR_LOG_LEVEL_DEBUG 5
#define IR_LOG_LEVEL_TRACE 6

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132
#endif

#ifndef M_PI
#define M_PI 3.1415926536
#endif

char *my_copystr (const char *str);
char *my_copynstr(const char *str, size_t max_length);

unsigned int unicode_len(const unsigned short *txt);

typedef struct time_check_interval {
    time_t last_checked;
    int check_seconds;
} time_check_interval;

int check_time_interval(time_check_interval *i);
time_check_interval *time_check_interval_new(int check_seconds);
void time_check_interval_free(time_check_interval *i);

typedef struct network_interface {
    char *ifname;
    char *ipaddress;
} network_interface;

typedef struct network_interfaces {
    int n;
    network_interface **interfaces;
} network_interfaces;

int check_internet (void);
network_interfaces *get_network_interfaces();
void free_network_interface(network_interface *interface);
void free_network_interfaces(network_interfaces *interfaces);

void base_close(void);
void base_init(const char *appname, FILE *dflt_log_file, int log_level);

#endif
