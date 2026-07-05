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
 * Lightweight helpers for standalone C tests.
 */

#ifndef VE301_TEST_H
#define VE301_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef int (*ve301_test_func)(void);
typedef int (*ve301_test_func_with_args)(int argc, char **argv);

typedef struct ve301_test_case {
    const char *name;
    const char *description;
    ve301_test_func run;
} ve301_test_case;

typedef struct ve301_test_case_with_args {
    const char *name;
    const char *description;
    ve301_test_func_with_args run;
} ve301_test_case_with_args;

#define TEST(name, description) static int name(void)
#define TEST_CASE(name, description) {#name, description, name}

#define TEST_WITH_ARGS(name, description) static int name(int argc, char **argv)
#define TEST_CASE_WITH_ARGS(name, description) {#name, description, name}

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #condition); \
            return 0; \
        } \
    } while (0)

#define ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, message); \
            return 0; \
        } \
    } while (0)

int ve301_run_test_cases(const ve301_test_case *cases, size_t num_cases);
int ve301_run_test_cases_with_args(const ve301_test_case_with_args *cases,
                                   size_t num_cases,
                                   int argc,
                                   char **argv);

#define TEST_MAIN(...) \
    int main(void) { \
        static const ve301_test_case test_cases[] = { __VA_ARGS__ }; \
        return ve301_run_test_cases(test_cases, sizeof(test_cases) / sizeof(test_cases[0])); \
    }

#define TEST_MAIN_ARGS(...) \
    int main(int argc, char **argv) { \
        static const ve301_test_case_with_args test_cases[] = { __VA_ARGS__ }; \
        return ve301_run_test_cases_with_args(test_cases, \
                                              sizeof(test_cases) / sizeof(test_cases[0]), \
                                              argc, \
                                              argv); \
    }

#endif
