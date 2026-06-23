WITH_BLUETOOTH=1
WITH_SPOTIFY=1
WITH_ALSA=1
ARCH=x86_64-linux-gnu
ifeq ($(FSANITIZE),1)
	ADD_CFLAGS+=-fsanitize=address
endif
include ../Makefile
CC=/usr/bin/x86_64-linux-gnu-gcc
ifeq ($(FSANITIZE),1)
	LDFLAGS=-fsanitize=address
endif
