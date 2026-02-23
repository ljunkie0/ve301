#include <stdlib.h>
#include "../base.h"
#include "playlist.h"

int playlist_add_song(playlist *p, song *s) {
    p->n_songs++;
    p->songs = realloc(p->songs, (p->n_songs * sizeof(song *)));
    p->songs[p->n_songs - 1] = s;
    return 1;
}

playlist *playlist_new(char *name) {
    playlist *p = malloc(sizeof(playlist));

    p->n_songs = 0;
    p->songs = 0;

    if (name) {
        p->name = my_copystr(name);
    } else {
        p->name = NULL;
    }
    return p;
}

int playlist_clear(playlist *p) {
    if (p->songs) {
        for (unsigned int i = 0; i < p->n_songs; i++) {
            song *s = p->songs[i];
            song_free(s);
            p->songs[i] = 0;
        }
        free (p->songs);
        p->songs = NULL;
        p->n_songs = 0;
    }

    return 0;
}

int playlist_dispose(playlist *p) {
    for (unsigned int i = 0; i < p->n_songs; i++) {
        song *s = p->songs[i];
        song_free(s);
        p->songs[i] = 0;
    }
    if (p->name)
        free((void *)p->name);
    free (p);
    return 0;
}
