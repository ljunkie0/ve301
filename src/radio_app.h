#ifndef VE3_1_H
#define VE3_1_H

#define DEFAULT_WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define NO_OF_SCALES 60
#define DEFAULT_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define DEFAULT_X_OFFSET 0
#define DEFAULT_Y_OFFSET 0
#define DEFAULT_LABEL_RADIUS 100
#define DEFAULT_SCALES_RADIUS_START 120
#define DEFAULT_SCALES_RADIUS_END 200
#define DEFAULT_ANGLE_OFFSET 0.0
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_INFO_FONT_SIZE 24
#define CALLBACK_SECONDS 5
#define CHECK_INTERNET_SECONDS 1
#define INFO_MENU_SECONDS 14
#define INFO_MENU_ITEM_SECONDS 5
#define RADIO_MENU_SEGMENTS_PER_ITEM 1
#define RADIO_MENU_NO_OF_LINES 3
#define RADIO_MENU_ITEMS_ON_SCALE_FACTOR 4

#define CHECK_RADIO_SECONDS 1
#ifdef BLUETOOTH
/**
 * Bluetooth
 */
#define CHECK_BLUETOOTH_SECONDS 1
#endif

#ifdef SPOTIFY
/**
 * Spotify
 */
#define CHECK_SPOTIFY_SECONDS 1
#endif

void radio_app_init(const char *app_name,
                    const char *radio_player_name,
                    const char *radio_player_label);
void radio_app_loop();
void radio_app_close();

#endif // VE3_1_H
