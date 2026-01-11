WIRINGPI=/home/chris/Dokumente/Development/WiringPi
#ARCH=arm-linux-gnueabihf
ARCH=aarch64-linux-gnu
ADDITIONAL_OBJS=rotaryencoder.o
VC=/opt/vc
ADDITIONAL_LIBS=-L$(VC)/lib -L$(WIRINGPI)/wiringPi -lwiringPi -lrt # -lvchiq_arm #-lbcm_host -lvcos -lvchiq_arm
#CFLAGS_ADDITIONAL=-Ofast -mtune=cortex-a53 -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -I $(WIRINGPI)/wiringPi -DRASPBERRY
#CFLAGS_ADDITIONAL=-Ofast -mtune=cortex-a53 -mcpu=cortex-a53 -I $(WIRINGPI)/wiringPi -DRASPBERRY
CFLAGS_ADDITIONAL=-mtune=cortex-a53 -mcpu=cortex-a53 -I $(WIRINGPI)/wiringPi -DRASPBERRY
LIBRS_ARCH=arm-linux-gnueabihf
DPKG_ARCH=:armhf
include ../Makefile
#CC=arm-linux-gnueabihf-gcc-10
CC=aarch64-linux-gnu-gcc-12
JNI_INCLUDES=-I /usr/lib/jvm/java-11-openjdk-arm64/include -I /usr/lib/jvm/java-11-openjdk-arm64/include/linux
#ADD_CFLAGS=-DPERF_TEST
LDFLAGS=

armhf-docker:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 sh -c "make CC=$(CC) WIFI_SCAN_DIRECTORY=/tmp/wifi-scan WITH_SPOTIFY=$(WITH_SPOTIFY) WITH_BLUETOOTH=$(WITH_BLUETOOTH) -C /mnt/ve301/Raspberry"

armhf-docker-clean:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../:/mnt/ve301" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 sh -c "make CC=$(CC) WIFI_SCAN_DIRECTORY=/tmp/wifi-scan -C /mnt/ve301/Raspberry clean"

armhf-docker-test-menu:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/../../:/mnt/ve301" \
			--volume "$(WIRINGPI):/mnt/WiringPi" \
			--volume "/opt/vc:/mnt/vc" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 make CC=$(CC) -C /mnt/ve301/ve301.ui/Raspberry WIRINGPI=/mnt/WiringPi VC=/mnt/vc test_menu
