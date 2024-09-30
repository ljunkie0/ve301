FROM debian:bullseye

ENV INSIDE_DOCKER_CONTAINER=1 \
    DEBIAN_FRONTEND=noninteractive \
    DEBCONF_NOWARNINGS=yes \
    PKG_CONFIG_ALLOW_CROSS=1 \
    PKG_CONFIG_PATH="/usr/lib/arm-linux-gnueabihf/pkgconfig"

RUN mkdir /build \
    && dpkg --add-architecture armhf \
    && apt-get update \
    && apt-get -y upgrade \
    && apt-get install -y --no-install-recommends \
        build-essential \
        crossbuild-essential-armhf \
        gcc-10-arm-linux-gnueabi \
        libsdl2-dev:armhf \
        libsdl2-ttf-dev:armhf \
        libsdl2-gfx-dev:armhf \
        libsdl2-image-dev:armhf \
        libmpdclient-dev:armhf \
        libcurl4-gnutls-dev:armhf \
        libcjson-dev:armhf \
        openjdk-11-jdk-headless:armhf \
        bc \
        dpkg-dev \
        liblzma-dev \
        pkg-config \
        gettext-base \
	libmnl-dev \
        && rm -rf /var/lib/apt/lists/*
