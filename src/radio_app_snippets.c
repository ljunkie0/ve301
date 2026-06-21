
void update_album_menu(char *artist) {
    menu_clear(app->album_menu);
    unsigned int a;
    char **albums = !artist ? get_albums(&a) : get_artist_albums(artist, &a);
    if (albums != NULL) {
        log_config(MAIN_CTX, "Album successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_config(MAIN_CTX, "Getting album %s\n", albums[r]);
            menu_item *mi = menu_add_sub_menu(app->album_menu, albums[r], app->song_menu, NULL);
            menu_item_set_object_type(mi, OBJ_TYPE_ALBUM);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: album list is NULL\n");
        menu_item_new(app->album_menu,
                      "Could not retrieve albums",
                      NULL,
                      NULL,
                      UNKNOWN_OBJECT_TYPE,
                      NULL,
                      -1,
                      NULL,
                      NULL,
                      -1);
    }
}

void update_artist_menu() {
    menu_clear(app->artist_menu);
    unsigned int a;
    char **artists = get_artists(&a);
    if (artists != NULL) {
        log_config(MAIN_CTX, "Artists successfully received\n");
        unsigned int r = 0;
        for (r = 0; r < a; r++) {
            log_config(MAIN_CTX, "Getting artists %d\n", r);
            menu_item *mi = menu_add_sub_menu(app->artist_menu, artists[r], app->album_menu, NULL);
            menu_item_set_object_type(mi, OBJ_TYPE_ARTIST);
        }
    } else {
        log_error(MAIN_CTX, "create_menu: artists list is NULL\n");
        menu_item_new(app->artist_menu,
                      "Could not retrieve artists",
                      NULL,
                      NULL,
                      UNKNOWN_OBJECT_TYPE,
                      NULL,
                      -1,
                      NULL,
                      NULL,
                      -1);
    }
}

void update_song_menu(char *album, char *artist) {
    log_config(MAIN_CTX, "Update song menu: album = %s, artist = %s\n", album, artist);
    menu_clear(app->song_menu);

    playlist *album_songs = NULL;
    if (album) {
        if (artist) {
            album_songs = get_artist_album_songs(artist, album);
        } else {
            album_songs = get_album_songs(album);
        }
    } else if (artist) {
        album_songs = get_artist_songs(artist);
    }

    if (album_songs != NULL) {
        unsigned int r = 0;
        for (r = 0; r < album_songs->n_songs; r++) {
            song *s = album_songs->songs[r];
            song *menu_song = song_clone(s);
            log_info(MAIN_CTX, "Song: %s\n", s->title);
            menu_item_new((menu *) app->song_menu,
                          menu_song ? menu_song->title : s->title,
                          NULL,
                          menu_song,
                          OBJ_TYPE_SONG,
                          NULL,
                          -1,
                          NULL,
                          NULL,
                          -1);
        }
        playlist_free(album_songs);
    } else {
        log_error(MAIN_CTX, "update_song_menu: playlist is NULL\n");
        menu_item_new((menu *) app->song_menu,
                      "FEHLER",
                      NULL,
                      NULL,
                      UNKNOWN_OBJECT_TYPE,
                      NULL,
                      -1,
                      NULL,
                      NULL,
                      -1);
    }
}

static int add_to_playlist_action(menu_event evt, menu *m, menu_item *item) {
    (void) m;
    (void) item;

    radio_app_touch_activity();

    if (evt != ACTIVATE && evt != ACTIVATE_1) {
        return 0;
    }

    song *current = get_playing_song();
    if (!current || !current->url || !current->url[0]) {
        log_warning(MAIN_CTX, "No current song to add to Radio playlist\n");
        song_free(current);
        return 0;
    }

    const char *label = radio_app_current_song_label(current);
    if (!label) {
        label = current->url;
    }

    add_radio_playlist_url(current->url, label);
    song_free(current);

    return 0;
}
