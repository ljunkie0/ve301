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
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
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

void init_log_file(const char *appname, FILE *dflt_log_file);
void close_log_file();
int log_level_enabled(const int log_ctx, const int lvl);
void set_log_level(const int log_ctx, int lvl);
int get_log_level(const int log_ctx);

#endif // LOGGING_H
