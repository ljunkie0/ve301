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

#ifndef RADIO_BROWSER_H
#define RADIO_BROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct radio_browser_entry {
    char *label;
    char *value;
    unsigned int stationcount;
} radio_browser_entry;

typedef struct radio_browser_entry_list {
    radio_browser_entry **entries;
    unsigned int n_entries;
} radio_browser_entry_list;

typedef struct radio_browser_station {
    char *stationuuid;
    char *name;
    char *url;
    char *url_resolved;
    char *homepage;
    char *favicon;
    char *tags;
    char *countrycode;
    char *language;
    char *languagecodes;
    char *codec;
    int bitrate;
    int lastcheckok;
} radio_browser_station;

typedef struct radio_browser_station_list {
    radio_browser_station **stations;
    unsigned int n_stations;
} radio_browser_station_list;

int radio_browser_init(const char *user_agent, const char *preferred_server);
void radio_browser_cleanup(void);

radio_browser_entry_list *radio_browser_get_local_entries(const char *countrycode, unsigned int limit);
radio_browser_entry_list *radio_browser_get_tags(unsigned int limit);
radio_browser_entry_list *radio_browser_get_languages(unsigned int limit);

radio_browser_station_list *radio_browser_get_local_stations(const char *countrycode,
                                                             unsigned int limit);
radio_browser_station_list *radio_browser_get_stations_by_tag(const char *tag, unsigned int limit);
radio_browser_station_list *radio_browser_get_stations_by_language(const char *language,
                                                                   unsigned int limit);

char *radio_browser_resolve_stream_url(const radio_browser_station *station);

void radio_browser_entry_free(radio_browser_entry *entry);
radio_browser_entry *radio_browser_entry_clone(const radio_browser_entry *entry);
void radio_browser_entry_list_free(radio_browser_entry_list *list);
void radio_browser_station_free(radio_browser_station *station);
radio_browser_station *radio_browser_station_clone(const radio_browser_station *station);
void radio_browser_station_list_free(radio_browser_station_list *list);

#ifdef __cplusplus
}
#endif

#endif
