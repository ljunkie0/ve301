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
#include "player.h"
#include "../base/util.h"
#include <stdlib.h>

player *player_new(const char *name,
                   const char *icon,
                   const char *label,
                   int seconds_to_check,
                   update_player_func *update) {
    player *p = malloc(sizeof(player));
    p->name = my_copystr(name);
    p->status = 0;
    p->previous_status = 0;
    p->icon = my_copystr(icon);
    p->label = my_copystr(label);
    p->album = NULL;
    p->artist = NULL;
    p->title = NULL;
    p->cover_uri = NULL;
    p->check_interval = time_check_interval_new(seconds_to_check);
    p->update = update;
    return p;
}

void player_free(player *p) {
    if (!p) {
        return;
    }
    if (p->name) {
        free(p->name);
    }
    if (p->label) {
        free(p->label);
    }
    if (p->icon) {
        free(p->icon);
    }
    if (p->check_interval) {
        time_check_interval_free(p->check_interval);
    }
    free (p);
}

void player_set_status(player *p, int status) {
    p->status = status;
    if (!status) {
        if (p->album) {
            free (p->album);
            p->album = NULL;
        }
        if (p->artist) {
            free (p->artist);
            p->artist = NULL;
        }
        if (p->title) {
            free (p->title);
            p->title = NULL;
        }
        if (p->cover_uri) {
            free (p->cover_uri);
            p->cover_uri = NULL;
        }
    }
}

char *player_get_name(player *p) {
    return p->name;
}

int player_update(player *p) {
    if (p->update) {
        return p->update();
    }
    return 1
        ;
}

int player_get_status(player *p) {
    return p->status;
}

int player_status_changed(player *p) {
    if (p->status != p->previous_status) {
        p->previous_status = p->status;
        return 1;
    }
    return 0;
}

char *player_get_icon(player *p) {
    return p->icon;
}

char *player_get_label(player *p) {
    return p->label;
}

void player_set_label(player *p, char *label) {
    if (p->label) {
        free(p->label);
    }
    p->label = label;
}
int player_do_check(player *p) {
    return check_time_interval(p->check_interval);
}

void player_set_album(player *p, char *album) {
    if (p->album) {
        free (p->album);
    }
    p->album = my_strdup(album);
}

char *player_get_album(player *p) {
    return p->album;
}

void player_set_artist(player *p, char *artist) {
    if (p->artist) {
        free (p->artist);
    }
    p->artist = my_strdup(artist);
}

char *player_get_artist(player *p) {
    return p->artist;
}

void player_set_title(player *p, char *title) {
    if (p->title) {
        free (p->title);
    }
    p->title = my_strdup(title);
}

char *player_get_title(player *p) {
    return p->title;
}

void player_set_cover_uri(player *p, char *cover_uri) {
    if (p->cover_uri) {
        free (p->cover_uri);
    }
    p->cover_uri = my_strdup(cover_uri);
}

char *player_get_cover_uri(player *p) {
    return p->cover_uri;
}
