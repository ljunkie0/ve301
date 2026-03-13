WITH_BLUETOOTH=0
WITH_SPOTIFY=1
ARCH=x86_64-linux-gnu
ifeq ($(FSANITIZE),1)
	ADD_CFLAGS+=-fsanitize=address
endif
include ../Makefile
ifeq ($(FSANITIZE),1)
	LDFLAGS=-fsanitize=address
endif
