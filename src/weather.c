/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include"base.h"
#include"weather.h"
#include<curl/curl.h>
#include<curl/easy.h>
#include<cjson/cJSON.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>

#define WEATHER_BASE_URL "http://api.openweathermap.org/data/2.5/weather"

typedef struct response {
    size_t length;
    char *text;
} response;

size_t write_fnc(char *ptr, size_t size, size_t nmemb, void *userdata) {
    response *r = (response *) userdata;
    r->length = nmemb;
    r->text = ptr;
    return nmemb;
}

static char *__weather_url;
CURL *__weather_curl;
time_t __weather_update_interval;
static time_t __weather_last_update_t = 0;
weather *__weather_last_result = NULL;

static pthread_t __weather_thread;

#define DAY_SUNNY 0xF00D
#define DAY_CLOUDY 0xF002
#define DAY_CLOUDY_GUSTS 0xF000
#define DAY_CLOUDY_WINDY 0xF001
#define DAY_FOG 0xF003
#define DAY_HAIL 0xF004
#define DAY_HAZE 0xF0B6
#define DAY_LIGHTNING 0xF005
#define DAY_RAIN 0xF008
#define DAY_RAIN_MIX 0xF006
#define DAY_RAIN_WIND 0xF007
#define DAY_SHOWERS 0xF009
#define DAY_SLEET 0xF0B2
#define DAY_SLEET_STORM 0xF068
#define DAY_SNOW 0xF00A
#define DAY_SNOW_THUNDERSTORM 0xF06B
#define DAY_SNOW_WIND 0xF065
#define DAY_SPRINKLE 0xF00B
#define DAY_STORM_SHOWERS 0xF00E
#define DAY_SUNNY_OVERCAST 0xF00C
#define DAY_THUNDERSTORM 0xF010
#define DAY_WINDY 0xF085
#define SOLAR_ECLIPSE 0xF06E
#define HOT 0xF072
#define DAY_CLOUDY_HIGH 0xF07D
#define DAY_LIGHT_WIND 0xF0C4
#define CLOUD 0XF041
#define CLOUDY 0XF013
#define CLOUDY_GUSTS 0XF011
#define CLOUDY_WINDY 0XF012
#define FOG 0XF014
#define HAIL 0XF015
#define RAIN 0XF019
#define RAIN_MIX 0XF017
#define RAIN_WIND 0XF018
#define SHOWERS 0XF01A
#define SLEET 0XF0B5
#define SNOW 0XF01B
#define SPRINKLE 0XF01C
#define STORM_SHOWERS 0XF01D
#define THUNDERSTORM 0XF01E
#define SNOW_WIND 0XF064
#define SNOW 0XF01B
#define SMOG 0XF074
#define SMOKE 0XF062
#define LIGHTNING 0XF016
#define RAINDROPS 0XF04E
#define RAINDROP 0XF078
#define DUST 0XF063
#define SNOWFLAKE_COLD 0XF076
#define WINDY 0XF021
#define STRONG_WIND 0XF050
#define SANDSTORM 0XF082
#define EARTHQUAKE 0XF0C6
#define FIRE 0XF0C7
#define FLOOD 0XF07C
#define METEOR 0XF071
#define TSUNAMI 0XF0C5
#define VOLCANO 0XF0C8
#define HURRICANE 0XF073
#define TORNADO 0XF056
#define SMALL_CRAFT_ADVISORY 0XF0CC
#define GALE_WARNING 0XF0CD
#define STORM_WARNING 0XF0CE
#define HURRICANE_WARNING 0XF0CF
#define WIND_DIRECTION 0XF0B1
#define NIGHT_CLEAR 0XF02E
#define NIGHT_ALT_CLOUDY 0XF086
#define NIGHT_ALT_CLOUDY_GUSTS 0XF022
#define NIGHT_ALT_CLOUDY_WINDY 0XF023
#define NIGHT_ALT_HAIL 0XF024
#define NIGHT_ALT_LIGHTNING 0XF025
#define NIGHT_ALT_RAIN 0XF028
#define NIGHT_ALT_RAIN_MIX 0XF026
#define NIGHT_ALT_RAIN_WIND 0XF027
#define NIGHT_ALT_SHOWERS 0XF029
#define NIGHT_ALT_SLEET 0XF0B4
#define NIGHT_ALT_SLEET_STORM 0XF06A
#define NIGHT_ALT_SNOW 0XF02A
#define NIGHT_ALT_SNOW_THUNDERSTORM 0XF06D
#define NIGHT_ALT_SNOW_WIND 0XF067
#define NIGHT_ALT_SPRINKLE 0XF02B
#define NIGHT_ALT_STORM_SHOWERS 0XF02C
#define NIGHT_ALT_THUNDERSTORM 0XF02D
#define NIGHT_CLOUDY 0XF031
#define NIGHT_CLOUDY_GUSTS 0XF02F
#define NIGHT_CLOUDY_WINDY 0XF030
#define NIGHT_FOG 0XF04A
#define NIGHT_HAIL 0XF032
#define NIGHT_LIGHTNING 0XF033
#define NIGHT_PARTLY_CLOUDY 0XF083
#define NIGHT_RAIN 0XF036
#define NIGHT_RAIN_MIX 0XF034
#define NIGHT_RAIN_WIND 0XF035
#define NIGHT_SHOWERS 0XF037
#define NIGHT_SLEET 0XF0B3
#define NIGHT_SLEET_STORM 0XF069
#define NIGHT_SNOW 0XF038
#define NIGHT_SNOW_THUNDERSTORM 0XF06C
#define NIGHT_SNOW_WIND 0XF066
#define NIGHT_SPRINKLE 0XF039
#define NIGHT_STORM_SHOWERS 0XF03A
#define NIGHT_THUNDERSTORM 0XF03B
#define LUNAR_ECLIPSE 0XF070
#define STARS 0XF077
#define STORM_SHOWERS 0XF01D
#define THUNDERSTORM 0XF01E
#define NIGHT_ALT_CLOUDY_HIGH 0XF07E
#define NIGHT_CLOUDY_HIGH 0XF080
#define NIGHT_ALT_PARTLY_CLOUDY 0XF081

char *get_day_weather_icon_character(u_int16_t id) {

    switch(id) {
    case 200:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 201:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 202:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 210:
        return to_utf8(DAY_LIGHTNING,NULL);
    case 211:
        return to_utf8(DAY_LIGHTNING,NULL);
    case 212:
        return to_utf8(DAY_LIGHTNING,NULL);
    case 221:
        return to_utf8(DAY_LIGHTNING,NULL);
    case 230:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 231:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 232:
        return to_utf8(DAY_THUNDERSTORM,NULL);
    case 300:
        return to_utf8(DAY_SPRINKLE,NULL);
    case 301:
        return to_utf8(DAY_SPRINKLE,NULL);
    case 302:
        return to_utf8(DAY_RAIN,NULL);
    case 310:
        return to_utf8(DAY_RAIN,NULL);
    case 311:
        return to_utf8(DAY_RAIN,NULL);
    case 312:
        return to_utf8(DAY_RAIN,NULL);
    case 313:
        return to_utf8(DAY_RAIN,NULL);
    case 314:
        return to_utf8(DAY_RAIN,NULL);
    case 321:
        return to_utf8(DAY_SPRINKLE,NULL);
    case 500:
        return to_utf8(DAY_SPRINKLE,NULL);
    case 501:
        return to_utf8(DAY_RAIN,NULL);
    case 502:
        return to_utf8(DAY_RAIN,NULL);
    case 503:
        return to_utf8(DAY_RAIN,NULL);
    case 504:
        return to_utf8(DAY_RAIN,NULL);
    case 511:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 520:
        return to_utf8(DAY_SHOWERS,NULL);
    case 521:
        return to_utf8(DAY_SHOWERS,NULL);
    case 522:
        return to_utf8(DAY_SHOWERS,NULL);
    case 531:
        return to_utf8(DAY_STORM_SHOWERS,NULL);
    case 600:
        return to_utf8(DAY_SNOW,NULL);
    case 601:
        return to_utf8(DAY_SLEET,NULL);
    case 602:
        return to_utf8(DAY_SNOW,NULL);
    case 611:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 612:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 615:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 616:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 620:
        return to_utf8(DAY_RAIN_MIX,NULL);
    case 621:
        return to_utf8(DAY_SNOW,NULL);
    case 622:
        return to_utf8(DAY_SNOW,NULL);
    case 701:
        return to_utf8(DAY_FOG,NULL);
    case 711:
        return to_utf8(SMOKE,NULL);
    case 721:
        return to_utf8(DAY_HAZE,NULL);
    case 731:
        return to_utf8(DUST,NULL);
    case 741:
        return to_utf8(DAY_FOG,NULL);
    case 761:
        return to_utf8(DUST,NULL);
    case 762:
        return to_utf8(DUST,NULL);
    case 781:
        return to_utf8(TORNADO,NULL);
    case 800:
        return to_utf8(DAY_SUNNY,NULL);
    case 801:
        return to_utf8(DAY_CLOUDY,NULL);
    case 802:
        return to_utf8(DAY_CLOUDY,NULL);
    case 803:
        return to_utf8(CLOUDY,NULL);
    case 804:
        return to_utf8(CLOUDY,NULL);
    case 900:
        return to_utf8(TORNADO,NULL);
    case 902:
        return to_utf8(HURRICANE,NULL);
    case 903:
        return to_utf8(SNOWFLAKE_COLD,NULL);
    case 904:
        return to_utf8(HOT,NULL);
    case 906:
        return to_utf8(DAY_HAIL,NULL);
    case 957:
        return to_utf8(STRONG_WIND,NULL);
    }
    return to_utf8(DAY_SUNNY,NULL);
}

char *get_night_weather_icon_character(u_int16_t id) {

    switch(id) {

    case 200:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 201:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 202:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 210:
        return to_utf8(NIGHT_ALT_LIGHTNING,NULL);
    case 211:
        return to_utf8(NIGHT_ALT_LIGHTNING,NULL);
    case 212:
        return to_utf8(NIGHT_ALT_LIGHTNING,NULL);
    case 221:
        return to_utf8(NIGHT_ALT_LIGHTNING,NULL);
    case 230:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 231:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 232:
        return to_utf8(NIGHT_ALT_THUNDERSTORM,NULL);
    case 300:
        return to_utf8(NIGHT_ALT_SPRINKLE,NULL);
    case 301:
        return to_utf8(NIGHT_ALT_SPRINKLE,NULL);
    case 302:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 310:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 311:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 312:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 313:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 314:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 321:
        return to_utf8(NIGHT_ALT_SPRINKLE,NULL);
    case 500:
        return to_utf8(NIGHT_ALT_SPRINKLE,NULL);
    case 501:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 502:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 503:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 504:
        return to_utf8(NIGHT_ALT_RAIN,NULL);
    case 511:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 520:
        return to_utf8(NIGHT_ALT_SHOWERS,NULL);
    case 521:
        return to_utf8(NIGHT_ALT_SHOWERS,NULL);
    case 522:
        return to_utf8(NIGHT_ALT_SHOWERS,NULL);
    case 531:
        return to_utf8(NIGHT_ALT_STORM_SHOWERS,NULL);
    case 600:
        return to_utf8(NIGHT_ALT_SNOW,NULL);
    case 601:
        return to_utf8(NIGHT_ALT_SLEET,NULL);
    case 602:
        return to_utf8(NIGHT_ALT_SNOW,NULL);
    case 611:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 612:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 615:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 616:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 620:
        return to_utf8(NIGHT_ALT_RAIN_MIX,NULL);
    case 621:
        return to_utf8(NIGHT_ALT_SNOW,NULL);
    case 622:
        return to_utf8(NIGHT_ALT_SNOW,NULL);
    case 701:
        return to_utf8(NIGHT_FOG,NULL);
    case 711:
        return to_utf8(SMOKE,NULL);
    case 721:
        return to_utf8(DAY_HAZE,NULL);
    case 731:
        return to_utf8(DUST,NULL);
    case 741:
        return to_utf8(NIGHT_FOG,NULL);
    case 761:
        return to_utf8(DUST,NULL);
    case 762:
        return to_utf8(DUST,NULL);
    case 781:
        return to_utf8(TORNADO,NULL);
    case 800:
        return to_utf8(NIGHT_CLEAR,NULL);
    case 801:
        return to_utf8(NIGHT_ALT_PARTLY_CLOUDY,NULL);
    case 802:
        return to_utf8(NIGHT_ALT_CLOUDY,NULL);
    case 803:
        return to_utf8(CLOUDY,NULL);
    case 804:
        return to_utf8(CLOUDY,NULL);
    case 900:
        return to_utf8(TORNADO,NULL);
    case 902:
        return to_utf8(HURRICANE,NULL);
    case 903:
        return to_utf8(SNOWFLAKE_COLD,NULL);
    case 904:
        return to_utf8(HOT,NULL);
    case 906:
        return to_utf8(NIGHT_ALT_HAIL,NULL);
    case 957:
        return to_utf8(STRONG_WIND,NULL);
    }
    return to_utf8(NIGHT_CLEAR,NULL);
}

int init_weather(time_t update_interval, const char *api_key, const char *location, const char *units) {

    int l1 = strlen(WEATHER_BASE_URL);
    int l2 = strlen(api_key) + 7;
    int l3 = strlen(location) + 3;
    int l4 = strlen(units) + 7;

    __weather_url = malloc((l1+l2+l3+l4+1) * sizeof(char));
    __weather_url[0] = 0;

    strcat(__weather_url,WEATHER_BASE_URL);

    strcat(__weather_url,"?units=");
    strcat(__weather_url,units);

    strcat(__weather_url,"&q=");
    strcat(__weather_url,location);

    strcat(__weather_url,"&appid=");
    strcat(__weather_url,api_key);

    log_info(WEATHER_CTX,"Weather URL: %s\n", __weather_url);

    __weather_update_interval = update_interval;
    __weather_last_result = malloc(sizeof(weather));
    __weather_last_result->temp = -0.0;
    __weather_last_result->weather_icon = to_utf8(DAY_SUNNY,NULL);
    return 0;

}

int cleanup_weather() {

    /* always cleanup */
    if (__weather_last_result) {
        free(__weather_last_result);
    }
    return 1;

}

weather *get_weather() {

    time_t timer;
    time(&timer);

    if (timer - __weather_last_update_t > __weather_update_interval) {

        __weather_last_update_t = timer;

        __weather_curl = curl_easy_init();

        if(__weather_curl) {
            response *r = malloc(sizeof(response));
            r->length = 0;
            r->text = NULL;

            curl_easy_setopt(__weather_curl, CURLOPT_URL, __weather_url);
            /* Forcing HTTP/3 will make the connection fail if the server isn't
                                                            accessible over QUIC + HTTP/3 on the given host and port.
                                                            Consider using CURLOPT_ALTSVC instead! */
            curl_easy_setopt(__weather_curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2_0);
            curl_easy_setopt(__weather_curl, CURLOPT_WRITEFUNCTION, &write_fnc);
            curl_easy_setopt(__weather_curl, CURLOPT_WRITEDATA, r);

            /* Perform the request, res will get the return code */
            CURLcode res = curl_easy_perform(__weather_curl);

            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } else {

                char *main_str = NULL;
                double main_id = 0;
                double temp = 0.0;

                cJSON *json = cJSON_Parse(r->text);
                cJSON *weather_array = cJSON_GetObjectItem(json,"weather");
                if (cJSON_GetArraySize(weather_array)) {
                    cJSON *weather_obj = cJSON_GetArrayItem(weather_array,0);
                    cJSON *weather_main_obj = cJSON_GetObjectItem(weather_obj,"main");
                    main_str = cJSON_GetStringValue(weather_main_obj);
                    main_id = cJSON_GetNumberValue(weather_main_obj);
                    cJSON *weather_main_id_obj = cJSON_GetObjectItem(weather_obj,"id");
                    if (weather_main_id_obj) {
                        main_id = cJSON_GetNumberValue(weather_main_id_obj);
                    }
                }

                cJSON *main_obj = cJSON_GetObjectItem(json,"main");
                if (main_obj) {
                    cJSON *temp_obj = cJSON_GetObjectItem(main_obj,"temp");
                    temp = cJSON_GetNumberValue(temp_obj);
                }

                if (main_str) {
                    if (__weather_last_result) {
                        if (__weather_last_result->weather_icon) {
                            free(__weather_last_result->weather_icon);
                        }
                        free(__weather_last_result);
                    }
                    __weather_last_result = malloc(sizeof(weather));
                    __weather_last_result->weather_icon = get_day_weather_icon_character(main_id);
                    __weather_last_result->temp = temp;
                }

                cJSON_Delete(json);
            }
            free(r);
            curl_easy_cleanup(__weather_curl);
        }
    }

    return __weather_last_result;

}

void *on_weather_thread_start(void *__weather_listener) {
    log_info(WEATHER_CTX, "Weather thread successfully started\n");

    weather_listener *listener = (weather_listener *) __weather_listener;

    while (1) {

        weather *weather = get_weather();
        listener(weather);
        sleep(5);

    }

    return NULL;
}

void start_weather_thread(weather_listener *listener) {
    int r = pthread_create(&__weather_thread, NULL, on_weather_thread_start, listener);
    if (r) {
        log_error(WEATHER_CTX, "Could not start weather thread: %d\n", r);
    }
}
