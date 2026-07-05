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

#include "log_contexts.h"

static char *__log_context_names[]
    = {"BASE", "MAIN", "MENU", "MPD", "ENCODER", "SDL", "WHEATHER", "BLUETOOTH", "SPOTIFY"};

char *get_log_context_name(enum log_context context) {
	if (context < 0 || context >= NUM_CTX) {
		return "UNKNOWN";
	}
	return __log_context_names[context];
}
