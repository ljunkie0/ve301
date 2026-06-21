/*
 * VE301
 *
 * Shared runner implementation for standalone C tests.
 */

#include "test.h"

int ve301_run_test_cases(const ve301_test_case *cases, size_t num_cases) {
    size_t i;

    for (i = 0; i < num_cases; ++i) {
        if (!cases[i].run()) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int ve301_run_test_cases_with_args(const ve301_test_case_with_args *cases,
                                   size_t num_cases,
                                   int argc,
                                   char **argv) {
    size_t i;

    for (i = 0; i < num_cases; ++i) {
        if (!cases[i].run(argc, argv)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
