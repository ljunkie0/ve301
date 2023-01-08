WIRINGPI=/home/chris/Dokumente/Development/WiringPi
ARCH=arm-linux-gnueabihf
ADDITIONAL_OBJS=rotaryencoder.o
ADDITIONAL_LIBS=-L/opt/vc/lib -L$(WIRINGPI)/wiringPi -lwiringPi -lrt -lbcm_host -lvcos -lvchiq_arm
CFLAGS_ADDITIONAL=-mtune=cortex-a53 -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -I $(WIRINGPI)/wiringPi -DRASPBERRY
LIBRS_ARCH=arm-linux-gnueabihf
DPKG_ARCH=:armhf
include ../Makefile.general
