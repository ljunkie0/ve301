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

#include "radio_app/radio_app.h"
#include <string.h>

int main(int argc, char **argv) {
    int verbosity_level = 1;

    if (argc > 1) {
        if (argv[1][0] == '-') {
            int l = strlen(argv[1]);
            for (int c = 1; c < l; c++) {
                if (argv[1][c] == 'v') {
                    verbosity_level++;
                }
            }
        }
    } else {
        verbosity_level = -1;
    }

    radio_app_init("ve301", "VE301", "VE 301", verbosity_level);
    radio_app_loop();
    radio_app_close();
    return 0;
}
