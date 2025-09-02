#include "player.h"

player *player_new(
    char *icon,
    char *label,
    int seconds_to_check
) {
    player *p = malloc(sizeof(player));
    p->status = 0;
    p->icon = icon;
    p->label = label;
    p->check_interval = time_check_interval_new(seconds_to_check);
    return p;
}

void player_free(player *p) {
    if (!p) {
        return;
    }
    if (p->check_interval) {
        free(p->check_interval);
    }
    free(p);
}
void player_set_status(player *p, int status) {
    p->status = status;
}
int player_get_status(player *p) {
    return p->status;
}
char *player_get_icon(player *p) {
    return p->icon;
}
void player_set_icon(player *p, char *icon) {
    p->icon = icon;
}
char *player_get_label(player *p) {
    return p->label;
}
void player_set_label(player *p, char *label) {
    p->label = label;
}
int player_do_check(player *p) {
    return check_time_interval(p->check_interval);
}
void player_set_check_seconds(player *p, int check_seconds) {
    p->check_interval->check_seconds = check_seconds;
}
