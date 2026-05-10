/*
 * VE301
 *
 * Small standalone test for the logging formatter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../base/log_contexts.h"
#include "../base/logging.h"

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

int main(int argc, char **argv) {
    FILE *target = stdout;

    if (argc > 1 && argv[1] && argv[1][0] != '\0' && strcmp(argv[1], "-") != 0) {
        target = fopen(argv[1], "w");
        if (!target) {
            perror("fopen");
            return EXIT_FAILURE;
        }
    }

    init_log_file("logging_output_test", target);
    emit_sample_logs();
    close_log_file();

    if (target != stdout) {
        fclose(target);
    }

    return EXIT_SUCCESS;
}
