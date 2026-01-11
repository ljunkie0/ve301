FROM debian:bookworm

ENV INSIDE_DOCKER_CONTAINER=1 \
    DEBIAN_FRONTEND=noninteractive \
    DEBCONF_NOWARNINGS=yes \
    PKG_CONFIG_ALLOW_CROSS=1 \
    PKG_CONFIG_PATH="/usr/lib/arm-linux-gnueabihf/pkgconfig"

RUN mkdir /build \
    && dpkg --add-architecture arm64 \
    && apt-get update \
    && apt-get -y upgrade \
    && apt-get install -y --no-install-recommends \
        build-essential \
        crossbuild-essential-arm64 \
	libdbus-1-dev:arm64 \
        libsdl2-dev:arm64 \
        libsdl2-ttf-dev:arm64 \
        libsdl2-gfx-dev:arm64 \
        libsdl2-image-dev:arm64 \
        libmpdclient-dev:arm64 \
        libcurl4-gnutls-dev:arm64 \
        libcjson-dev:arm64 \
	libwebsockets-dev:arm64 \
	libmnl-dev:arm64 \
        bc \
        dpkg-dev \
        liblzma-dev \
        pkg-config \
        gettext-base \
	wget \
	git \
    && cd /tmp \
    && wget --no-check-certificate https://github.com/WiringPi/WiringPi/releases/download/3.16/wiringpi_3.16_arm64.deb && dpkg -i wiringpi_3.16_arm64.deb \
    && ln -sf /usr/lib/libwiringPi.so.3.16 /usr/lib/libwiringPi.so \
    && ln -sf /usr/lib/libwiringPiDev.so.3.16 /usr/lib/libwiringPiDev.so \
    && GIT_SSL_NO_VERIFY=true git clone https://github.com/bmegli/wifi-scan.git \
    && cd wifi-scan \
    && make CC=aarch64-linux-gnu-gcc-12 \
    && rm -rf /var/lib/apt/lists/*
