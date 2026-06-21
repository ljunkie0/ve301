#ifndef PLAYER_IF_H
#define PLAYER_IF_H

#include "player.h"
void player_set_active(player *p, int active);
void player_set_album(player *p, char *album);
void player_set_artist(player *p, char *artist);
void player_set_title(player *p, char *title);
int player_set_cover_uri(player *p, char *cover_uri);
void player_set_info_bg_path(player *p, const char *path);
void player_set_song_id(player *p, long song_id);
void player_set_cover_image_path(player *p, const char *path);
void player_set_playback_status(player *p, player_status status);
int player_thread_running(player *p);

#endif // PLAYER_IF_H
