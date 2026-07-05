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

#define _GNU_SOURCE

#include <time.h>
#include <sys/time.h>

#include <pthread.h>
#include "logging.h"
#include "log_contexts.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *log_file;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *LOG_TIME_FMT = "%Y-%m-%d %H:%M:%S";
static const int LOG_CONTEXT_WIDTH = 9;
static const int LOG_LEVEL_WIDTH = 7;

static int _log_levels[NUM_CTX];

static char *__log_level_colors[] = {
    "\x1b[0m",  // NO LEVEL
    "\x1b[1;31m", // ERROR
    "\x1b[33m", // WARNING
    "\x1b[32m", // INFO
    "\x1b[36m", // CONFIG
    "\x1b[34m", // DEBUG
    "\x1b[0m"   // TRACE
};

static const char *__log_level_names[] = {
    "OFF",
    "ERROR",
    "WARNING",
    "INFO",
    "CONFIG",
    "DEBUG",
    "TRACE"
};

void init_log_file(const char *appname, FILE *dflt_log_file) {
    if (dflt_log_file) {
        pthread_mutex_lock(&log_mutex);
        log_file = dflt_log_file;
        pthread_mutex_unlock(&log_mutex);
    } else {
        char *logfile_path = my_catstr("/tmp/", my_catstr(appname, ".log"));
        fprintf(stderr, "Logfile: %s\n", logfile_path);
        FILE *new_log_file = fopen(logfile_path, "w");
        if (!new_log_file) {
            fprintf(stderr, "Could not create file %s\n", logfile_path);
        } else {
            setlinebuf(new_log_file);
            pthread_mutex_lock(&log_mutex);
            log_file = new_log_file;
            pthread_mutex_unlock(&log_mutex);
            log_info(BASE_CTX, "log file created\n");
        }
        free(logfile_path);
    }
}

void close_log_file() {
    pthread_mutex_lock(&log_mutex);
    if (log_file && (log_file != stderr) && (log_file != stdout)) {
        fclose(log_file);
    }
    log_file = NULL;
    pthread_mutex_unlock(&log_mutex);
}

int log_level_enabled(const int log_ctx, const int lvl) {
    int enabled;

    pthread_mutex_lock(&log_mutex);
    enabled = log_file && (_log_levels[log_ctx] >= lvl);
    pthread_mutex_unlock(&log_mutex);

    return enabled;
}

void set_log_level(const int log_ctx, const int lvl) {
    pthread_mutex_lock(&log_mutex);
    _log_levels[log_ctx] = lvl;
    pthread_mutex_unlock(&log_mutex);
}

int get_log_level(const int log_ctx) {
    int lvl;

    pthread_mutex_lock(&log_mutex);
    lvl = _log_levels[log_ctx];
    pthread_mutex_unlock(&log_mutex);

    return lvl;
}

const char *get_log_level_name(int level) {
    if (level < 0 || level >= (int) (sizeof(__log_level_names) / sizeof(__log_level_names[0]))) {
        return "UNKNOWN";
    }

    return __log_level_names[level];
}

void log_variable_args(const int log_ctx,
                       const int lvl,
                       const char *__restrict __format,
                       va_list args) {
    pthread_mutex_lock(&log_mutex);

    if (!log_file) {
        log_file = stderr;
    }

    if (log_file && (_log_levels[log_ctx] >= lvl)) {
        char *log_context_name = get_log_context_name(log_ctx);
        char *log_level_color = __log_level_colors[lvl];
        const char *log_level_name = get_log_level_name(lvl);

        time_t t;
        struct tm tmp;
        struct timeval tv;
        char timebuf[40];

        gettimeofday(&tv, NULL);
        t = tv.tv_sec;
        if (localtime_r(&t, &tmp) && strftime(timebuf, sizeof(timebuf), LOG_TIME_FMT, &tmp)) {
            snprintf(timebuf + strlen(timebuf), sizeof(timebuf) - strlen(timebuf), ".%03ld", tv.tv_usec / 1000);
            fprintf(log_file, "%s[%s] [%-*s] %-*s: ",
                    log_level_color,
                    timebuf,
                    LOG_CONTEXT_WIDTH,
                    log_context_name,
                    LOG_LEVEL_WIDTH,
                    log_level_name);
        } else {
            fprintf(log_file, "%s[%-*s] %-*s: ",
                    log_level_color,
                    LOG_CONTEXT_WIDTH,
                    log_context_name,
                    LOG_LEVEL_WIDTH,
                    log_level_name);
        }

        vfprintf(log_file, __format, args);
        fputs(__log_level_colors[0], log_file);
    }

    pthread_mutex_unlock(&log_mutex);
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
