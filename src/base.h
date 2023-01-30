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

#ifndef BASE_H
#define BASE_H

#include<stdio.h>
#include<inttypes.h>
#include"log_contexts.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL 5
#endif

#define IR_LOG_LEVEL_OFF -1
#define IR_LOG_LEVEL_ERROR 0
#define IR_LOG_LEVEL_WARNING 1
#define IR_LOG_LEVEL_INFO 2
#define IR_LOG_LEVEL_CONFIG 3
#define IR_LOG_LEVEL_DEBUG 4
#define IR_LOG_LEVEL_TRACE 5

char *my_copystr (const char *str);
char *my_copynstr (const char *str, unsigned int max_length);
char *get_name_from_path(const char *path);
int index_of(const char *str, const char chr);

unsigned int unicode_len(const unsigned short *txt);
unsigned short *unicode_copy(const unsigned short *txt);

void __log_error(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_ERROR
#define log_error(...) __log_error(__VA_ARGS__)
#else
#define log_error(...)
#endif

void __log_warning(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_WARNING
#define log_warning(...) __log_warning(__VA_ARGS__)
#else
#define log_warning(...)
#endif

void __log_info(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_INFO
#define log_info(...) __log_info(__VA_ARGS__)
#else
#define log_info(...)
#endif

void __log_config(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_CONFIG
#define log_config(...) __log_config(__VA_ARGS__)
#else
#define log_config(...)
#endif

void __log_debug(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_DEBUG
#define log_debug(...) __log_debug(__VA_ARGS__)
#else
#define log_debug(...)
#endif

void __log_trace(const int log_ctx, const char *__restrict __format, ...);
#if LOG_LEVEL >= IR_LOG_LEVEL_DEBUG
#define log_trace(...) __log_trace(__VA_ARGS__)
#else
#define log_trace(...)
#endif

int check_internet (void);

void set_config_value(char *key, char *value);
void set_config_value_int (char *key, int value);
void set_config_value_double (char *key, double value);

char *get_config_value(char *key, const char *dflt);
char *get_config_value_group(char *key, const char *dflt, const char *group);
int get_config_value_int(char *key, int dflt);
int get_config_value_int_group(char *key, int dflt, const char *group);
float get_config_value_float(char *key, float dflt);
float get_config_value_float_group(char *key, float dflt, const char *group);
double get_config_value_double(char *key, double dflt);
double get_config_value_double_group(char *key, double dflt, const char *group);

void base_close(void);
void base_init(const char *appname, FILE *dflt_log_file, int log_level);
void init_config_file(const char *appname);
void write_config ();

uint16_t *to_unicode(const char *txt, uint32_t *length, uint16_t **second_line, uint32_t *length2);
char *to_utf8(uint32_t utf, uint32_t *length);

#endif
