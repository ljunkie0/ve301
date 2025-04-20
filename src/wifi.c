#include <stdlib.h>
#include "wifi.h"
#include "base.h"

const char *bssid_to_string(const uint8_t bssid[BSSID_LENGTH], char bssid_string[BSSID_STRING_LENGTH]) {
    snprintf(bssid_string, BSSID_STRING_LENGTH, "%02x:%02x:%02x:%02x:%02x:%02x",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return bssid_string;
}

struct station_info *scan_wifi_station(char *interface) {

    struct wifi_scan *wifi=NULL;    //this stores all the library information
    struct station_info *station = malloc(sizeof(struct station_info));    //this is where we are going to keep information about AP (Access Point) we are connected to
    int status = 0;

    wifi=wifi_scan_init(interface);

    int tries = 0;

    while (tries++ < 5 && !status) {
        status = wifi_scan_station(wifi, station);
    }

    wifi_scan_close(wifi);

    if(status > 0) {
        return station;
    }

    return 0;

}
