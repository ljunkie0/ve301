WITH_SPOTIFY=1
WITH_BLUETOOTH=1

CC=$(ARCH)-gcc-11
CXX=$(ARCH)-g++-13
CFLAGS_DBUS=-I/usr/include/dbus-1.0 -I/usr/lib/$(ARCH)/dbus-1.0/include
LDFLAGS_DBUS=-ldbus-1
LDFLAGS=
STRIP=$(ARCH)-strip

MENU_OBJS=menu/glyph_obj.o menu/text_obj.o menu/menu.o
AUDIO_OBJS=audio/audio.o audio/alsa.o
OBJS=log_contexts.o base.o player.o sdl_util.o $(MENU_OBJS) $(AUDIO_OBJS) input_menu.o weather.o 
JNI_OBJS=java/org_ljunkie_ve301_Application.o java/org_ljunkie_ve301_MenuControl.o java/org_ljunkie_ve301_Menu.o java/org_ljunkie_ve301_MenuItem.o java/menu_jni.o
#JNI_INCLUDES=-I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include -I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include/linux
JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
JNI_INCLUDES=-I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/linux
LIBS_SDL=-lm -lSDL2 -lSDL2_ttf -lSDL2_gfx -lSDL2_image
LIB_MPD=-lmpdclient
LIB_WEATHER=-lcurl -lcjson -lpthread
LIB_WS=-lwebsockets
LIB_ASOUND=-lasound
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

WIFI_SCAN_DIRECTORY=../../wifi-scan

IR_LOG_LEVEL_OFF=-1
IR_LOG_LEVEL_ERROR=0
IR_LOG_LEVEL_WARNING=1
IR_LOG_LEVEL_INFO=2
IR_LOG_LEVEL_CONFIG=3
IR_LOG_LEVEL_DEBUG=4
IR_LOG_LEVEL_TRACE=5

LOG_LEVEL=${IR_LOG_LEVEL_TRACE}

CFLAGS=-Wall -O3 -DMPD_DEBUG -DLOG_LEVEL=${LOG_LEVEL} -fPIC ${ADD_CFLAGS}

CURL_LIB=/usr/lib/$(ARCH)/libcurl.so
MPD_LIB=/usr/lib/$(ARCH)/libmpdclient.so
SDL_LIB=/usr/lib/$(ARCH)/libSDL2.so
DBUS_LIB=/usr/lib/$(ARCH)/libdbus-1.so

all: ve301

ve301: $(OBJS) $(ADDITIONAL_OBJS) main.o wifi.o wifi_scan.o
	$(CC) -o ve301 $(LDFLAGS) $(OBJS) wifi.o wifi_scan.o $(ADDITIONAL_OBJS) main.o $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(LIB_SPOTIFY) $(ADDITIONAL_LIBS) $(LIB_ASOUND) -lmnl

wifi_scan.o: $(WIFI_SCAN_DIRECTORY)/wifi_scan.c
	make -C $(WIFI_SCAN_DIRECTORY)
	mv $(WIFI_SCAN_DIRECTORY)/wifi_scan.o .

bt_devices: bt_devices.o
	$(CC) -o bt_devices bt_devices.o $(LDFLAGS_DBUS)

bt_devices.o: ../src/bt_devices.c
	$(CC) -c bt_devices.o $(CFLAGS_DBUS) ../src/bt_devices.c

$(WIFI_SCAN_DIRECTORY)/wifi-scan-all:
	make -C $(WIFI_SCAN_DIRECTORY) wifi-scan-all

$(WIFI_SCAN_DIRECTORY)/wifi-scan-station:
	make -C $(WIFI_SCAN_DIRECTORY) wifi-scan-station
	
$(WIFI_SCAN_DIRECTORY)/wifi_scan.c:
	git clone https://github.com/bmegli/wifi-scan.git $(WIFI_SCAN_DIRECTORY)

libve301.so: $(SDL_LIB) $(JNI_OBJS) $(ADDITIONAL_OBJS) base.o sdl_util.o $(MENU_OBJS)
	$(CC) -shared -o libve301.so $(JNI_OBJS) base.o sdl_util.o $(MENU_OBJS) $(ADDITIONAL_OBJS) $(LIBS_SDL) $(ADDITIONAL_LIBS)

$(MPD_LIB):
	sudo apt-get -y install libmpdclient-dev$(DPKG_ARCH)

$(SDL_LIB):
	sudo apt-get -y install libsdl2-dev$(DPKG_ARCH) libsdl2-ttf-dev$(DPKG_ARCH) libsdl2-image-dev$(DPKG_ARCH) libsdl2-gfx-dev$(DPKG_ARCH)

$(CURL_LIB):
	sudo apt-get -y install libcurl4-openssl-dev$(DPKG_ARCH)

$(DBUS_LIB):
	sudo apt-get -y install libdbus-1-dev$(DPKG_ARCH)

debian-dependencies-install: $(SDL_LIB) $(MPD_LIB) $(CURL_LIB) $(DBUS_LIB)

%.o: ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu:
	mkdir menu

audio:
	mkdir audio

menu/%.o: ../src/menu/%.c ../src/menu/%.h menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

audio/%.o: ../src/audio/%.c ../src/audio/%.h audio
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

menu/examples/%.o: ../src/menu/%.c ../src/menu/%.h ../src/menu/examples/%.c ../src/menu/examples/%.h
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

main.o: ../src/main.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

audio/bluetooth.o: ../src/audio/bluetooth.c
	$(CC) ${CFLAGS_DBUS} $(CFLAGS) -c -o audio/bluetooth.o ../src/audio/bluetooth.c

audio/bluetooth: audio/bluetooth.o
	$(CC) -o bluetooth audio/bluetooth.o -ldbus-1

test_menu.o: ../src/test_menu.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"
	
test_menu: test_menu.o ${MENU_OBJS} base.o sdl_util.o $(ADDITIONAL_OBJS)
	$(CC) -o test_menu test_menu.o $(MENU_OBJS) base.o sdl_util.o $(LDFLAGS) $(ADDITIONAL_OBJS) $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(ADDITIONAL_LIBS)

menu/Menu.o: ../src/menu/Menu.cpp
	$(CXX) $(CFLAGS) -c -o $@ "$<"

menu/Menu%.o: ../src/menu/Menu%.cpp
	$(CXX) $(CFLAGS) -c -o $@ "$<"

mainObjective.o: base.o log_contexts.o sdl_util.o menu/menu.o menu/glyph_obj.o menu/text_obj.o ../src/mainObjective.cpp ../src/menu/MenuCtrl.cpp ../src/menu/Menu.cpp ../src/menu/MenuItem.cpp
	$(CXX) $(CFLAGS) -c ../src/mainObjective.cpp

mainObjective: mainObjective.o menu/menu.o menu/glyph_obj.o menu/text_obj.o menu/MenuCtrl.o menu/Menu.o menu/MenuItem.o base.o sdl_util.o log_contexts.o
	$(CXX) -o mainObjective mainObjective.o base.o sdl_util.o log_contexts.o menu/menu.o menu/glyph_obj.o menu/text_obj.o menu/MenuCtrl.o menu/Menu.o menu/MenuItem.o $(LIBS_SDL)

java/%.o: ../src/java/%.c ../src/java/%.h $(OBJS)
	mkdir -p java
	$(CC) $(JNI_INCLUDES) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"


clean:
	rm -f $(OBJS) main.o wifi.o wifi_scan.o $(ADDITIONAL_OBJS) $(JNI_OBJS) $(AUDIO_OBJS) libve301.so ve301 bt_devices
	rm -rf menu
	rm -rf audio
	make -C $(WIFI_SCAN_DIRECTORY) clean
