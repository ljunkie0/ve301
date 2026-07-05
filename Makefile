CC=$(ARCH)-gcc
CXX=$(ARCH)-g++
CFLAGS_DBUS=-I/usr/include/dbus-1.0 -I/usr/lib/$(ARCH)/dbus-1.0/include
LDFLAGS_DBUS=-ldbus-1
LDFLAGS=
STRIP=$(ARCH)-strip

BASE_OBJS=base/util.o base/logging.o base/log_contexts.o base/config.o
MENU_OBJS=menu/glyph_obj.o menu/text_obj.o menu/menu_menu.o menu/menu_ctrl.o menu/menu_item.o
AUDIO_OBJS=audio/player.o audio/mpd_media_player.o audio/song.o audio/playlist.o radio_browser/radio_browser.o
RADIO_APP_OBJS=radio_app/core.o radio_app/config.o radio_app/themes.o radio_app/players.o radio_app/info_menu.o radio_app/volume_menu.o radio_app/navigation_menu.o radio_app/network_menu.o radio_app/actions.o radio_app/theme.o
OBJS=$(RADIO_APP_OBJS) base/base.o util/sdl_util.o $(BASE_OBJS) $(MENU_OBJS) $(AUDIO_OBJS) radio_browser/menu.o input_menu/input_menu.o weather/weather.o
JNI_OBJS=java/org_ljunkie_ve301_Application.o java/org_ljunkie_ve301_MenuControl.o java/org_ljunkie_ve301_Menu.o java/org_ljunkie_ve301_MenuItem.o java/menu_jni.o
#JNI_INCLUDES=-I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include -I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include/linux
JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
JNI_INCLUDES=-I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/linux
LIBS_SDL=-lm -lSDL2 -lSDL2_ttf -lSDL2_gfx -lSDL2_image
LIB_MPD=-lmpdclient
LIB_WEATHER=-lcurl -lcjson -lpthread
LIB_WS=-lwebsockets

ifeq ($(WITH_ALSA),1)
	ADD_CFLAGS += -DALSA
	LIB_ASOUND = -lasound
	AUDIO_OBJS += audio/alsa.o
endif

ifeq ($(WITH_SPOTIFY),1)
	LIB_SPOTIFY=$(LIB_WS)
	ADD_CFLAGS += -DSPOTIFY
	AUDIO_OBJS += audio/spotify.o
endif

ifeq ($(WITH_BLUETOOTH),1)
	LIB_BT=-ldbus-1
	ADD_CFLAGS += -DBLUETOOTH
	AUDIO_OBJS += audio/bluetooth.o
endif

ifeq ($(DEBUG),1)
	CFLAGS_O=-g
else
	CFLAGS_O=-O3
endif

WIFI_SCAN_DIRECTORY=$(CURDIR)/wifi-scan

IR_LOG_LEVEL_OFF=-1
IR_LOG_LEVEL_ERROR=0
IR_LOG_LEVEL_WARNING=1
IR_LOG_LEVEL_INFO=2
IR_LOG_LEVEL_CONFIG=3
IR_LOG_LEVEL_DEBUG=4
IR_LOG_LEVEL_TRACE=5

LOG_LEVEL=${IR_LOG_LEVEL_TRACE}

CFLAGS = -I$(WIFI_SCAN_DIRECTORY) -Wall $(CFLAGS_O) -DLOG_LEVEL=${LOG_LEVEL} -fPIC ${ADD_CFLAGS}

CURL_LIB=/usr/lib/$(ARCH)/libcurl.so
MPD_LIB=/usr/lib/$(ARCH)/libmpdclient.so
SDL_LIB=/usr/lib/$(ARCH)/libSDL2.so
DBUS_LIB=/usr/lib/$(ARCH)/libdbus-1.so
WEBSOCKETS_LIB=/usr/lib/$(ARCH)/libwebsockets.so
CJSON_LIB=/usr/lib/$(ARCH)/libcjson.so
MNL_LIB=/usr/lib/$(ARCH)/libmnl.so

all: ve301

ve301: $(OBJS) $(ADDITIONAL_OBJS) main.o util/wifi.o $(WIFI_SCAN_DIRECTORY)/wifi_scan.o
	$(CC) -o ve301 $(LDFLAGS) $(OBJS) util/wifi.o $(WIFI_SCAN_DIRECTORY)/wifi_scan.o $(ADDITIONAL_OBJS) main.o $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(LIB_SPOTIFY) $(LIB_ASOUND) $(ADDITIONAL_LIBS) $(LIB_ASOUND) -lmnl

strip: ve301
	$(STRIP) ve301

bt_devices: bt_devices.o
	$(CC) -o bt_devices bt_devices.o $(LDFLAGS_DBUS)

bt_devices.o: ../src/bt_devices.c
	$(CC) -c bt_devices.o $(CFLAGS_DBUS) ../src/bt_devices.c

wifi_scan.o: $(WIFI_SCAN_DIRECTORY)/wifi_scan.c
	+make -C $(WIFI_SCAN_DIRECTORY)

$(WIFI_SCAN_DIRECTORY)/wifi-scan-all:
	+make -C $(WIFI_SCAN_DIRECTORY) wifi-scan-all

$(WIFI_SCAN_DIRECTORY)/wifi-scan-station:
	+make -C $(WIFI_SCAN_DIRECTORY) wifi-scan-station
	
$(WIFI_SCAN_DIRECTORY)/wifi_scan.h:
	GIT_SSL_NO_VERIFY=true git clone https://github.com/bmegli/wifi-scan.git $(WIFI_SCAN_DIRECTORY)

$(WIFI_SCAN_DIRECTORY)/wifi_scan.c: $(WIFI_SCAN_DIRECTORY)/wifi_scan.h
	
../src/util/wifi.h: $(WIFI_SCAN_DIRECTORY)/wifi_scan.h

radio_app:
	mkdir -p radio_app

radio_app/%.o: ../src/radio_app/%.c ../src/radio_app/private.h ../src/radio_app/radio_app.h ../src/util/wifi.h | radio_app
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

libve301.so: $(SDL_LIB) $(JNI_OBJS) $(ADDITIONAL_OBJS) base/base.o util/sdl_util.o $(MENU_OBJS)
	$(CC) -shared -o libve301.so $(JNI_OBJS) base/base.o util/sdl_util.o $(MENU_OBJS) $(ADDITIONAL_OBJS) $(LIBS_SDL) $(ADDITIONAL_LIBS)

$(MPD_LIB):
	sudo apt-get -y install libmpdclient-dev$(DPKG_ARCH)

$(SDL_LIB):
	sudo apt-get -y install libsdl2-dev$(DPKG_ARCH) libsdl2-ttf-dev$(DPKG_ARCH) libsdl2-image-dev$(DPKG_ARCH) libsdl2-gfx-dev$(DPKG_ARCH)

$(CURL_LIB):
	sudo apt-get -y install libcurl4-openssl-dev$(DPKG_ARCH)

$(DBUS_LIB):
	sudo apt-get -y install libdbus-1-dev$(DPKG_ARCH)

$(WEBSOCKETS_LIB):
	sudo apt-get -y install libwebsockets-dev$(DPKG_ARCH)

$(CJSON_LIB):
	sudo apt-get -y install libcjson-dev$(DPKG_ARCH)

$(MNL_LIB):
	sudo apt-get -y install libmnl-dev$(DPKG_ARCH)

debian-dependencies-install: $(SDL_LIB) $(MPD_LIB) $(CURL_LIB) $(DBUS_LIB) $(WEBSOCKETS_LIB) $(CJSON_LIB) $(MNL_LIB)

%.o: ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu:
	mkdir menu

audio:
	mkdir audio

base:
	mkdir base

util:
	mkdir -p util

input_menu:
	mkdir -p input_menu

weather:
	mkdir -p weather

radio_browser:
	mkdir -p radio_browser

raspberry:
	mkdir -p raspberry

test_dir:
	mkdir -p tests

.PHONY: tests
tests: logging_output_test logging_output_test_trace tests/audio/player_test.bin
	@fail=0; 	for test_cmd in $^; do 		if ./$$test_cmd; then 			printf '%-32s	PASS\n' "$$test_cmd"; 		else 			status=$$?; 			printf '%-32s	FAIL (exit %s)\n' "$$test_cmd" "$$status"; 			fail=1; 		fi; 	done; 	exit $$fail

menu/menu.o: ../src/menu/menu.c ../src/menu/menu.h | menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu/%_obj.o: ../src/menu/%_obj.c ../src/menu/%_obj.h | menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu/menu_%.o: ../src/menu/menu_%.c ../src/menu/menu_%.h ../src/menu/menu_%_priv.h | menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

audio/mpd_media_player.o: ../src/audio/mpd_media_player.c ../src/audio/media_player.h | audio
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

audio/%.o: ../src/audio/%.c ../src/audio/%.h | audio
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

base/%.o: ../src/base/%.c ../src/base/%.h | base
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

raspberry/%.o: ../src/raspberry/%.c ../src/raspberry/%.h | raspberry
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

radio_browser/%.o: ../src/radio_browser/%.c ../src/radio_browser/%.h | radio_browser
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

weather/%.o: ../src/weather/%.c ../src/weather/%.h | weather
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

input_menu/%.o: ../src/input_menu/%.c ../src/input_menu/%.h | input_menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

util/%.o: ../src/util/%.c ../src/util/%.h | util
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu/examples/%.o: ../src/menu/%.c ../src/menu/%.h ../src/menu/examples/%.c ../src/menu/examples/%.h
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

main.o: ../src/main.c $(WIFI_SCAN_DIRECTORY)/wifi_scan.h 
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

audio/bluetooth.o: ../src/audio/bluetooth.c
	$(CC) ${CFLAGS_DBUS} $(CFLAGS) -c -o audio/bluetooth.o ../src/audio/bluetooth.c

audio/bluetooth: audio/bluetooth.o
	$(CC) -o bluetooth audio/bluetooth.o -ldbus-1

tests/test.o: ../src/tests/test.c ../src/tests/test.h | test_dir
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

tests/logging_output_test.o: ../src/tests/logging_output_test.c ../src/tests/test.h | test_dir
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

logging_output_test: tests/test.o tests/logging_output_test.o base/logging.o base/log_contexts.o base/util.o
	$(CC) -o logging_output_test tests/test.o tests/logging_output_test.o base/logging.o base/log_contexts.o base/util.o $(LDFLAGS) -lpthread -lm

tests/logging_output_test_trace.o: ../src/tests/logging_output_test.c ../src/tests/test.h | test_dir
	$(CC) $(filter-out -DLOG_LEVEL=5,$(CFLAGS)) -DLOG_LEVEL=6 $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

logging_output_test_trace: tests/test.o tests/logging_output_test_trace.o base/logging.o base/log_contexts.o base/util.o
	$(CC) -o logging_output_test_trace tests/test.o tests/logging_output_test_trace.o base/logging.o base/log_contexts.o base/util.o $(LDFLAGS) -lpthread -lm

tests/audio/player/player_test.o: ../src/tests/audio/player/player_test.c ../src/tests/test.h | tests/audio/player
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

tests/audio:
	mkdir -p tests/audio

tests/audio/player:
	mkdir -p tests/audio/player

tests/audio/player_test.bin: tests/test.o tests/audio/player/player_test.o audio/player.o base/base.o base/config.o base/util.o base/logging.o base/log_contexts.o | tests/audio
	$(CC) -o tests/audio/player_test.bin tests/test.o tests/audio/player/player_test.o audio/player.o base/base.o base/config.o base/util.o base/logging.o base/log_contexts.o $(LDFLAGS) -lpthread -lm

tests/audio/player_test: tests/audio/player_test.bin
	@if ./$<; then 		printf 'PASS %s\n' '$@'; 	else 		status=$$?; 		printf 'FAIL %s (exit %s)\n' '$@' "$$status"; 		exit $$status; 	fi

test_menu.o: ../src/test_menu.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"
	
test_menu: test_menu.o ${MENU_OBJS} base/base.o util/sdl_util.o $(ADDITIONAL_OBJS)
	$(CC) -o test_menu test_menu.o $(MENU_OBJS) base/base.o util/sdl_util.o $(LDFLAGS) $(ADDITIONAL_OBJS) $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(ADDITIONAL_LIBS)

menu/cpp:
	mkdir -p menu/cpp

menu/cpp/%.o: ../src/menu/cpp/%.cpp | menu/cpp
	$(CXX) $(CFLAGS) -c -o $@ "$<"

mainObjective.o: base/base.o util/sdl_util.o $(MENU_OBJS) ../src/mainObjective.cpp ../src/menu/cpp/MenuCtrl.cpp ../src/menu/cpp/Menu.cpp ../src/menu/cpp/MenuItem.cpp
	$(CXX) $(CFLAGS) -c ../src/mainObjective.cpp

mainObjective: mainObjective.o menu/menu_menu.o menu/glyph_obj.o menu/text_obj.o menu/cpp/MenuCtrl.o menu/cpp/Menu.o menu/cpp/MenuItem.o base/base.o util/sdl_util.o base/config.o base/logging.o base/util.o base/log_contexts.o
	$(CXX) -o mainObjective mainObjective.o base/base.o util/sdl_util.o base/config.o base/logging.o base/util.o base/log_contexts.o $(MENU_OBJS) menu/cpp/MenuCtrl.o menu/cpp/Menu.o menu/cpp/MenuItem.o $(LIBS_SDL)

java/%.o: ../src/java/%.c ../src/java/%.h $(OBJS)
	mkdir -p java
	$(CC) $(JNI_INCLUDES) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"


clean:
	rm -f $(OBJS) main.o util/wifi.o $(WIFI_SCAN_DIRECTORY)/wifi_scan.o $(ADDITIONAL_OBJS) $(JNI_OBJS) libve301.so ve301 bt_devices
	rm -rf menu
	rm -rf radio_app
	rm -rf util
	rm -rf input_menu
	rm -rf radio_browser
	rm -rf weather
	rm -rf raspberry
	rm -rf audio
	rm -rf base 
	rm -rf tests
	+make -C $(WIFI_SCAN_DIRECTORY) clean
