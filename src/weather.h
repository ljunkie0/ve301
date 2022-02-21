/*
 * Copyright 2022 LJunkie
 * http://www.???.??
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

#ifndef __WEATHER_H
#define __WEATHER_H
#include<time.h>

typedef struct weather_struct {
    double temp;
    char *weather_icon;
} weather;

int init_weather(time_t update_interval, const char *api_key, const char *location, const char *units);
int cleanup_weather();
weather *get_weather();
void start_weather_thread();

typedef int weather_listener(weather *weather);

#endif
