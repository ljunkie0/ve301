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

/*
 * Split input into one or two newly allocated display lines.
 * max_chars is the maximum number of UTF-8 characters per line.
 * The caller owns line1 and line2; line2 is NULL for one-line output.
 * Returns 1 or 2 on success, 0 on allocation or argument errors.
 */
int split_lines(char *input, char **line1, char **line2, size_t max_chars);
int is_blank(const char *str);

void get_sinus_and_cosinus(int angle, double *cos, double *sin);
void util_cleanup(void);

long long current_time_millis();

#endif // UTIL_H
