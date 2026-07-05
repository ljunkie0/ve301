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

#ifndef VE301_PODCAST_H
#define VE301_PODCAST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct podcast_feed {
    char *name;
    char *url;
} podcast_feed;

typedef struct podcast_feed_list {
    podcast_feed **feeds;
    unsigned int n_feeds;
} podcast_feed_list;

typedef struct podcast_episode {
    char *name;
    char *date;
    char *url;
} podcast_episode;

typedef struct podcast_episode_list {
    podcast_episode **episodes;
    unsigned int n_episodes;
} podcast_episode_list;

podcast_feed_list *podcast_feed_list_load(const char *path);
podcast_episode_list *podcast_episode_list_load(const char *url, unsigned int limit);

void podcast_feed_free(podcast_feed *feed);
void podcast_feed_list_free(podcast_feed_list *list);
void podcast_episode_free(podcast_episode *episode);
void podcast_episode_list_free(podcast_episode_list *list);

#ifdef __cplusplus
}
#endif

#endif // VE301_PODCAST_H
