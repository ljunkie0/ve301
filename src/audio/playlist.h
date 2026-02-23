#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "song.h"

typedef struct playlist {
    const char *name;
    song **songs;
    unsigned int n_songs;
    unsigned int disposed;
} playlist;

int playlist_add_song(playlist *p, song *s);
playlist *playlist_new(char *name);
int playlist_clear(playlist *p);
int playlist_dispose(playlist *p);

#endif // PLAYLIST_H
