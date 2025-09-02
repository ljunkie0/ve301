#ifndef PLAYER_H
#define PLAYER_H
#include "base.h"
typedef struct player {
    int status;
    char *icon;
    char *label;
    time_check_interval *check_interval;
} player;

player *player_new(char *icon, char *label, int seconds_to_check);

void player_free(player *p);
int player_get_status(player *p);
void player_set_status(player *p, int status);
char *player_get_icon(player *p);
void player_set_icon(player *p, char *icon);
char *player_get_label(player *p);
void player_set_label(player *p, char *label);
void player_set_check_seconds(player *p, int check_seconds);

int player_do_check(player *p);
#endif
