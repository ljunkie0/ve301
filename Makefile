CC=$(ARCH)-gcc-11
CFLAGS_DBUS=-I/usr/include/dbus-1.0 -I/usr/lib/$(ARCH)/dbus-1.0/include
LDFLAGS=
STRIP=$(ARCH)-strip

MENU_OBJS=menu/glyph_obj.o menu/text_obj.o menu/menu.o
OBJS=base.o sdl_util.o $(MENU_OBJS) audio.o weather.o bluetooth.o
JNI_OBJS=java/org_ljunkie_ve301_Application.o java/org_ljunkie_ve301_MenuControl.o java/org_ljunkie_ve301_Menu.o java/org_ljunkie_ve301_MenuItem.o java/menu_jni.o
#JNI_INCLUDES=-I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include -I /usr/lib/jvm/java-1.17.0-openjdk-amd64/include/linux
JNI_INCLUDES=-I /usr/lib/jvm/default-java/include -I /usr/lib/jvm/default-java/include/linux
LIBS_SDL=-lm -lSDL2 -lSDL2_ttf -lSDL2_gfx -lSDL2_image
LIB_MPD=-lmpdclient
LIB_WEATHER=-lcurl -lcjson -lpthread
LIB_BT=-ldbus-1

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

all: ve301 libve301.so

ve301: $(OBJS) $(ADDITIONAL_OBJS)
	$(CC) -o ve301 $(LDFLAGS) $(OBJS) $(ADDITIONAL_OBJS) main.o $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(ADDITIONAL_LIBS)

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

menu/%.o: ../src/menu/%.c ../src/menu/%.h menu
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"

main.o: ../src/main.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ main.o "$<"

bluetooth.o: ../src/bluetooth.c
	$(CC) ${CFLAGS_DBUS} $(CFLAGS) -c ../src/bluetooth.c

bluetooth: bluetooth.o
	$(CC) -o bluetooth bluetooth.o -ldbus-1

test_menu.o: ../src/test_menu.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"
	
test_menu: test_menu.o ${MENU_OBJS} base.o sdl_util.o
	$(CC) -o test_menu test_menu.o $(MENU_OBJS) base.o sdl_util.o $(LDFLAGS) $(ADDITIONAL_OBJS) $(LIBS_SDL) $(LIB_MPD) $(LIB_WEATHER) $(LIB_BT) $(ADDITIONAL_LIBS)

java/%.o: ../src/java/%.c ../src/java/%.h $(OBJS)
	mkdir -p java
	$(CC) $(JNI_INCLUDES) $(CFLAGS) $(CFLAGS_ADDITIONAL) -c -o $@ "$<"


clean:
	rm -f $(OBJS) $(ADDITIONAL_OBJS) $(JNI_OBJS) libve301.so ve301
	rm -rf menu
