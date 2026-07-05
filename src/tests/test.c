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
