WIRINGPI=/home/chris/Dokumente/Development/WiringPi
#ARCH=arm-linux-gnueabihf
ARCH=aarch64-linux-gnu
ADDITIONAL_OBJS=rotaryencoder.o
ADDITIONAL_LIBS=-L$(WIRINGPI)/wiringPi -lwiringPi -lrt
CFLAGS_ADDITIONAL=-mtune=cortex-a53 -mcpu=cortex-a53 -I $(WIRINGPI)/wiringPi -DRASPBERRY
LIBRS_ARCH=arm-linux-gnueabihf
DPKG_ARCH=:armhf
ifeq ($(FSANITIZE),1)
        ADD_CFLAGS+=-fsanitize=address
endif

WITH_BLUETOOTH=1
WITH_SPOTIFY=1

include ../Makefile
#CC=arm-linux-gnueabihf-gcc-10
CC=aarch64-linux-gnu-gcc-12
JNI_INCLUDES=-I /usr/lib/jvm/java-11-openjdk-arm64/include -I /usr/lib/jvm/java-11-openjdk-arm64/include/linux
#ADD_CFLAGS=-DPERF_TEST
ifeq ($(FSANITIZE),1)
        LDFLAGS=-fsanitize=address
endif

armhf-docker:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 sh -c "make CC=$(CC) FSANITIZE=$(FSANITIZE) WITH_SPOTIFY=$(WITH_SPOTIFY) WITH_BLUETOOTH=$(WITH_BLUETOOTH) -C /mnt/ve301/Raspberry"

armhf-docker-strip:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 sh -c "make CC=$(CC) FSANITIZE=$(FSANITIZE) WITH_SPOTIFY=$(WITH_SPOTIFY) WITH_BLUETOOTH=$(WITH_BLUETOOTH) -C /mnt/ve301/Raspberry strip"

armhf-docker-clean:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 sh -c "make CC=$(CC) -C /mnt/ve301/Raspberry clean"

armhf-docker-test-menu:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 make CC=$(CC) -C /mnt/ve301/Raspberry test_menu
