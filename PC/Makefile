ARCH=x86_64-linux-gnu
ADD_CFLAGS=-O0 -g
ADD_CFLAGS+=-fsanitize=address
ADDITIONAL_LIBS=-lgcov
#ADDITIONAL_LIBS=-lc_p
include ../Makefile
LDFLAGS=-fsanitize=address

