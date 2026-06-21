#ifndef MPD_PLAYER_H
#define MPD_PLAYER_H

#include "player.h"
#include "playlist.h"

player *media_player_new(const char *name, char *icon, const char *label, const long check_millis);
playlist *media_player_get_internet_radios();
int media_player_get_volume();
void media_player_set_volume(int volume);

#endif // MPD_PLAYER_H
