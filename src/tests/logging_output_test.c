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

/*
 * VE301
 *
 * Small standalone test for the logging formatter.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../base/log_contexts.h"
#include "../base/logging.h"
#include "test.h"

static int validate_timestamp_with_millis(const char *line) {
    const char *ts_start = strstr(line, "m[");
    size_t len;
    int i;

    if (!ts_start) {
        return 0;
    }

    ts_start += 2;
    len = strlen(ts_start);
    if (len < 24) {
        return 0;
    }

    for (i = 0; i < 4; ++i) {
        if (!isdigit((unsigned char) ts_start[i])) {
            return 0;
        }
    }

    if (ts_start[4] != '-' ||
        !isdigit((unsigned char) ts_start[5]) ||
        !isdigit((unsigned char) ts_start[6]) ||
        ts_start[7] != '-' ||
        !isdigit((unsigned char) ts_start[8]) ||
        !isdigit((unsigned char) ts_start[9]) ||
        ts_start[10] != ' ' ||
        !isdigit((unsigned char) ts_start[11]) ||
        !isdigit((unsigned char) ts_start[12]) ||
        ts_start[13] != ':' ||
        !isdigit((unsigned char) ts_start[14]) ||
        !isdigit((unsigned char) ts_start[15]) ||
        ts_start[16] != ':' ||
        !isdigit((unsigned char) ts_start[17]) ||
        !isdigit((unsigned char) ts_start[18]) ||
        ts_start[19] != '.' ||
        !isdigit((unsigned char) ts_start[20]) ||
        !isdigit((unsigned char) ts_start[21]) ||
        !isdigit((unsigned char) ts_start[22]) ||
        ts_start[23] != ']') {
        return 0;
    }

    return 1;
}

static void emit_sample_logs(void) {
    set_log_level(BASE_CTX, IR_LOG_LEVEL_TRACE);
    set_log_level(MAIN_CTX, IR_LOG_LEVEL_TRACE);

    log_error(BASE_CTX, "error sample\n");
    log_warning(BASE_CTX, "warning sample\n");
    log_info(BASE_CTX, "info sample\n");
    log_config(BASE_CTX, "config sample\n");
    log_debug(BASE_CTX, "debug sample: %s %d\n", "value", 42);
    log_trace(BASE_CTX, "trace sample\n");
}

static int verify_output(FILE *target) {
    char line[1024];
    int lines_seen = 0;
#if LOG_LEVEL >= IR_LOG_LEVEL_TRACE
    const int expected_lines = 6;
#else
    const int expected_lines = 5;
#endif

    rewind(target);
    while (fgets(line, sizeof(line), target)) {
        if (strcmp(line, "\x1b[0m\n") == 0 || strcmp(line, "\x1b[0m") == 0) {
            continue;
        }

        ++lines_seen;
        if (!validate_timestamp_with_millis(line)) {
            fprintf(stderr, "unexpected log format: %s", line);
            return 0;
        }
    }

    if (lines_seen != expected_lines) {
        fprintf(stderr, "expected %d log lines, got %d\n", expected_lines, lines_seen);
        return 0;
    }

    return 1;
}

static int copy_output(FILE *source, const char *path) {
    char line[1024];
    FILE *copy;

    copy = fopen(path, "w");
    if (!copy) {
        perror("fopen");
        return 0;
    }

    rewind(source);
    while (fgets(line, sizeof(line), source)) {
        fputs(line, copy);
    }

    fclose(copy);
    return 1;
}

TEST_WITH_ARGS(logging_output_format, "formats log timestamps with millisecond precision") {
    char log_path[] = "/tmp/logging_output_test.XXXXXX";
    int fd;
    FILE *write_target;
    FILE *read_target;
    int result;

    fd = mkstemp(log_path);
    ASSERT_MSG(fd >= 0, "mkstemp failed");

    write_target = fdopen(fd, "w");
    if (!write_target) {
        perror("fdopen");
        close(fd);
        unlink(log_path);
        return 0;
    }

    init_log_file("logging_output_test", write_target);
    emit_sample_logs();
    fflush(write_target);
    close_log_file();

    read_target = fopen(log_path, "r");
    if (!read_target) {
        perror("fopen");
        unlink(log_path);
        return 0;
    }

    result = verify_output(read_target);
    if (result && argc > 1 && argv[1] && argv[1][0] != '\0' && strcmp(argv[1], "-") != 0) {
        result = copy_output(read_target, argv[1]);
    }

    fclose(read_target);
    unlink(log_path);
    return result;
}

TEST_MAIN_ARGS(TEST_CASE_WITH_ARGS(logging_output_format,
                                   "formats log timestamps with millisecond precision"));
