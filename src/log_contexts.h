/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef LOG_CONTEXTS
#define LOG_CONTEXTS

enum log_context {
	BASE_CTX	= 0,
	MAIN_CTX	= 1,
	MENU_CTX	= 2,
	AUDIO_CTX	= 3,
	ENCODER_CTX	= 4,
	SDL_CTX		= 5,
	WEATHER_CTX	= 6,
	BT_CTX		= 7,
	SPOTIFY_CTX	= 8,
	NUM_CTX		= 9
};

char *get_log_context_name(enum log_context context);

#endif
