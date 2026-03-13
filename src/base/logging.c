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

#include "logging.h"
#include "log_contexts.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *log_file;

static int _log_levels[NUM_CTX];

static char *__log_level_colors[] = {
    "\x1b[0m",  // NO LEVEL
    "\x1b[31m", // ERROR
    "\x1b[33m", // WARNING
    "\x1b[35m", // INFO
    "\x1b[0m",  // CONFIG
    "\x1b[0m",  // DEBUG
    "\x1b[0m"   // TRACE
};

void init_log_file(const char *appname, FILE *dflt_log_file) {
    if (!dflt_log_file) {
        char *logfile_path = my_catstr("/tmp/", my_catstr(appname, ".log"));
        fprintf(stderr, "Logfile: %s\n", logfile_path);
        log_file = fopen(logfile_path, "w");

        log_error(BASE_CTX, "log file created\n");
        if (!log_file) {
            fprintf(stderr, "Could not create file %s\n", logfile_path);
        } else {
            setlinebuf(log_file);
        }
        free(logfile_path);
    }
}

void close_log_file() {
    if (log_file && (log_file != stderr) && (log_file != stdout)) {
        fclose(log_file);
    }
}

int log_level_enabled(const int log_ctx, const int lvl) {
    return log_file && (_log_levels[log_ctx] >= lvl);
}

void set_log_level(const int log_ctx, const int lvl) {
    _log_levels[log_ctx] = lvl;
}

int get_log_level(const int log_ctx) {
    return _log_levels[log_ctx];
}

void log_variable_args(const int log_ctx,
                       const int lvl,
                       const char *__restrict __format,
                       va_list args) {
    if (!log_file) {
        log_file = stderr;
    }

    if (log_file && (_log_levels[log_ctx] >= lvl)) {
        char *log_context_name = get_log_context_name(log_ctx);
        char *log_level_color = __log_level_colors[lvl];

        int ll = strlen(log_level_color);
        int cl = strlen(log_context_name);
        int rl = strlen(__log_level_colors[0]);
        int fl = strlen(__format);
        char ctx_format[ll + cl + 2 + fl + rl + 1];

        memcpy(ctx_format, log_level_color, ll);
        memcpy(ctx_format + ll, log_context_name, cl);
        ctx_format[ll + cl] = ':';
        ctx_format[ll + cl + 1] = ' ';
        memcpy(ctx_format + ll + cl + 2, __format, fl);
        memcpy(ctx_format + ll + cl + 2 + fl, __log_level_colors[0], rl);
        ctx_format[ll + cl + 2 + fl + rl] = 0;
        vfprintf(log_file, ctx_format, args);
    }
}

void __log_error(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_ERROR, __format, args);
    va_end(args);
}

void __log_warning(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_WARNING, __format, args);
    va_end(args);
}

void __log_info(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_INFO, __format, args);
    va_end(args);
}

void __log_config(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_CONFIG, __format, args);
    va_end(args);
}

void __log_debug(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_DEBUG, __format, args);
    va_end(args);
}

void __log_trace(const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start(args, __format);
    log_variable_args(log_ctx, IR_LOG_LEVEL_TRACE, __format, args);
    va_end(args);
}
