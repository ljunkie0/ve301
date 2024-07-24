WIRINGPI=/home/chris/Dokumente/Development/WiringPi
ARCH=arm-linux-gnueabihf
ADDITIONAL_OBJS=rotaryencoder.o
VC=/opt/vc
ADDITIONAL_LIBS=-L$(VC)/lib -L$(WIRINGPI)/wiringPi -lwiringPi -lrt -lbcm_host -lvcos -lvchiq_arm
CFLAGS_ADDITIONAL=-Ofast -mtune=cortex-a53 -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -I $(WIRINGPI)/wiringPi -DRASPBERRY
LIBRS_ARCH=arm-linux-gnueabihf
DPKG_ARCH=:armhf
include ../Makefile
CC=arm-linux-gnueabihf-gcc-10
JNI_INCLUDES=-I /usr/lib/jvm/java-11-openjdk-armhf/include -I /usr/lib/jvm/java-11-openjdk-armhf/include/linux
#ADD_CFLAGS=-DPERF_TEST
LDFLAGS=

armhf-docker:
	docker build -t ve301 .
	docker run \
			--rm \
			--volume "$(CURDIR)/..:/mnt/ve301" \
			--volume "$(WIRINGPI):/mnt/WiringPi" \
			--volume "/opt/vc:/mnt/vc" \
			--env PERMFIX_UID="$$(id -u)" \
			--env PERMFIX_GID="$$(id -g)" \
			--env ARCHITECTURE="armhf" \
			ve301 make -C /mnt/ve301/Raspberry WIRINGPI=/mnt/WiringPi VC=/mnt/vc
