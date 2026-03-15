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
#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <stddef.h>

void free_and_set_null(void **p);

void strshift(char *str, int shift, int offset);
char *my_copystr(const char *str);
char *my_copynstr(const char *str, size_t max_length);

uint16_t *to_unicode(const char *txt, uint32_t *length, uint16_t **second_line, uint32_t *length2);
char *to_utf8(uint32_t utf, uint32_t *length);

char *my_catstr(const char *str1, const char *str2);
char *my_cat3str(const char *str1, const char *str2, const char *str3);
int my_strcmp(const char *str1, const char *str2);
char *my_strdup(const char *str);
int is_blank(const char *str);

void get_sinus_and_cosinus(int angle, double *cos, double *sin);
void util_cleanup(void);

#endif // UTIL_H
