#ifndef SONG_H
#define SONG_H

#include<limits.h>

const static unsigned int unknown_song_id = UINT_MAX;

typedef struct song {
    char *artist;
    char *name;
    char *title;
    const char *url;
    unsigned int id;
} song;

song *song_new(unsigned int id, const char *url, const char *name, const char *title);
void song_free(song *s);

#endif // SONG_H
