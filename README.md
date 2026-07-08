VE301 is a tiny program acting as an internet radio. It provides
an MPD client and an SDL window to choose the radio stations among
other things.
The display mimics the scale disk of a vintage radio like the ve301.
It runs on a raspberry pi as well as on a normal desktop (though for
the latter there are better mpd clients).
The project was inspired by https://amrhein.eu/Radio2. The original page is no
longer available.

In addition to being an mpd client it can also display the current weather
condition, browse Radio Browser stations, play configured podcast feeds, and
reacts to the bluetooth daemon for devices that connect as audio source.

It is still a work in progress and needs a lot of clean up. I also
guess there are still memory leaks and I am not sure whether the way
I use SDL is correct or optimal. On a Raspberry PI 3B (with SDL in framebuffer)
it is reasonably fast. The performance heavily depends on the config, e.g.
bump mapping slows down the most.

## Building

Go into the appropriate build folder (PC for desktop, Raspberry for Raspberry PI)
and type make.
This will probably fail as there are some dependencies to be installed. For convenience,
if you are running a debian system, you can try "make debian-dependencies-install".
This will install the dependencies
 - libsdl2-dev
 - libsdl2-image-dev
 - libsdl2-ttf-dev
 - libsdl2-gfx-dev
 - libcurl4-openssl-dev
 - libdbus-1-dev
 - libmpdclient-dev
 - libcjson-dev
 - libwebsockets-dev
 - libmnl-dev
 - libxml2-dev

For Raspberry PI you need a running docker instead. Within the Raspberry folder type
  
    sudo make armhf-docker

To clean

    sudo make armhf-docker-clean
    
First run will take some time as it creates the appropriate docker image.

CMake:
Desktop builds can also be done with CMake:

    cmake --preset desktop
    cmake --build --preset desktop

For a Raspberry cross-build with CMake on Ubuntu, install the host build tools first:

    sudo apt install cmake ninja-build pkg-config wget git build-essential

Then enable ARM64 packages and install the same cross-build toolchain and target
libraries that the Docker image uses:

    sudo dpkg --add-architecture arm64
    sudo apt update
    sudo apt install crossbuild-essential-arm64
    sudo apt install \
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
      libxml2-dev:arm64

WiringPi is required for Raspberry builds.

If you use the existing Docker-based Raspberry build, nothing extra is needed for
WiringPi. [Raspberry/Dockerfile](Raspberry/Dockerfile)
already installs the ARM64 WiringPi package.

If you build outside Docker, install the same ARM64 WiringPi package manually:

    wget https://github.com/WiringPi/WiringPi/releases/download/3.16/wiringpi_3.16_arm64.deb
    sudo apt install ./wiringpi_3.16_arm64.deb
    sudo ln -sf /usr/lib/libwiringPi.so.3.16 /usr/lib/libwiringPi.so
    sudo ln -sf /usr/lib/libwiringPiDev.so.3.16 /usr/lib/libwiringPiDev.so

Then export the cross pkg-config environment:

    export PKG_CONFIG_ALLOW_CROSS=1
    export PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig

Now configure and build:

    cmake --preset raspberry
    cmake --build --preset raspberry

## Web UI

The Makefile desktop and Raspberry builds enable the Mongoose-backed menu web
service by default. The first Makefile build clones Mongoose into the build
directory if it is missing. The service mirrors the menu tree in a browser.
It follows live menu and theme changes without reloading the full tree on every
poll. The current transient menu, such as Infos, Volume, or Messages, appears
in an adaptive panel at the bottom of the page. Volume can be changed with the
buttons in the top right corner; browsers that expose keyboard media keys can
also use `AudioVolumeUp` and `AudioVolumeDown`. Open the configured listen URL,
for example `http://localhost:8000/` on the desktop host.

The service is controlled by the VE301 config:

    menu_web_enabled=1
    menu_web_listen=http://0.0.0.0:8000

For CMake builds, enable it explicitly and point CMake at a Mongoose checkout
when neither PC/mongoose nor Raspberry/mongoose exists:

    cmake --preset desktop -DWITH_MENU_WEB=ON -DMONGOOSE_SOURCE_DIR=/path/to/mongoose
    cmake --build --preset desktop
 
## Spotify

Spotify support uses https://github.com/devgianlu/go-librespot as the Spotify
Connect client. VE301 does not play Spotify directly; it reads player state from
go-librespot and sends control requests to the go-librespot API server.

The Makefile desktop and Raspberry builds enable Spotify by default. For CMake
builds, use a preset with WITH_SPOTIFY=ON or pass `-DWITH_SPOTIFY=ON`.

go-librespot must run with its API server enabled on port 3678, for example in
`~/.config/go-librespot/config.yml`:

    server:
      enabled: true
      address: 127.0.0.1
      port: 3678

Then set the VE301 config:

    spotify_enabled=1
    spotify_host=127.0.0.1
    check_spotify_seconds=1
    spotify_icon=spotify_logo_48.png

If go-librespot runs on another host, set `spotify_host` to that host and make
sure its API server is reachable from VE301.

## Bluetooth

Bluetooth support uses the system DBus to observe BlueALSA and BlueZ state for
devices that connect as an audio sink. VE301 does not provide the Bluetooth
audio sink itself; BlueZ and BlueALSA must be installed, configured, and running
so that paired devices can stream audio to the machine. VE301 listens for DBus
events and shows connection and track metadata in the menu.

The Makefile desktop and Raspberry builds enable Bluetooth by default. For CMake
builds, pass `-DWITH_BLUETOOTH=ON` if the selected preset does not enable it.

Set the VE301 config:

    bluetooth_enabled=1
    check_bluetooth_seconds=1
    bluetooth_icon=bluetooth_logo_48.png

## Usage

  Create a directory ~/.ve301
  Copy sample-config/config to ~/.ve301/config
  If you want the Podcasts menu, set podcast_enabled=1 in ~/.ve301/config,
  copy sample-config/podcasts to ~/.ve301/podcasts, and edit the feed lines.
  Podcast feed lines use the format `Display name|RSS/Atom URL`.
  Make sure that MPD is running on the machine indicated as mpd_host in the config (default is localhost).
  The MPD server should have a playlist called [Radio Streams].m3u. Its content is shown in the Radio submenu.
  The name is chosen because with the old MPD client MPDroid you can add Stations via you mobile.

  On the desktop, use the mouse wheel to scroll through a menu, left click to activate an item / open a sub menu,
  right click to go back. Arrow keys Up and Down are used to increase/decrease volume.
  On the raspberry pi, the first button (Button A) replaces the mouse and the second (Button B) changes the volume.
  
  If you want the current weather conditions displayed you need to get a (free) api key from
  http://api.openweathermap.org/data/2.5/weather and provide it in the config file together with the location and the units.
