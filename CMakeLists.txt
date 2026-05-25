cmake_minimum_required(VERSION 3.20)

project(ve301 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_SPOTIFY "Build Spotify integration" ON)
option(WITH_BLUETOOTH "Build Bluetooth integration" OFF)
option(WITH_ALSA "Build ALSA integration" OFF)
option(FSANITIZE "Enable AddressSanitizer" OFF)
option(VE301_RASPBERRY "Enable Raspberry Pi specific sources and flags" OFF)
option(WITH_WIRINGPI "Build WiringPi-backed rotary encoder support" OFF)

set(LOG_LEVEL "5" CACHE STRING "Compile-time log level")
set(WIFI_SCAN_SOURCE_DIR "" CACHE PATH "Path to a wifi-scan checkout")
set(WIRINGPI_ROOT "" CACHE PATH "Path to the WiringPi project root")
set(WIRINGPI_INCLUDE_DIR "" CACHE PATH "Path to the WiringPi include directory")
set(WIRINGPI_LIBRARY "" CACHE FILEPATH "Path to the WiringPi library")

if(NOT WIFI_SCAN_SOURCE_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/PC/wifi-scan/wifi_scan.c")
        set(WIFI_SCAN_SOURCE_DIR "${CMAKE_SOURCE_DIR}/PC/wifi-scan")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/Raspberry/wifi-scan/wifi_scan.c")
        set(WIFI_SCAN_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Raspberry/wifi-scan")
    endif()
endif()

if(NOT EXISTS "${WIFI_SCAN_SOURCE_DIR}/wifi_scan.c")
    message(FATAL_ERROR
        "wifi-scan sources were not found. Set WIFI_SCAN_SOURCE_DIR to a checkout that contains wifi_scan.c.")
endif()

if(VE301_RASPBERRY AND NOT WITH_WIRINGPI)
    message(STATUS "Raspberry mode enabled without WiringPi. Rotary encoder support will be skipped.")
endif()

if(VE301_RASPBERRY AND WITH_WIRINGPI)
    set(_ve301_wiringpi_include_hints)
    set(_ve301_wiringpi_library_hints)

    if(WIRINGPI_ROOT)
        list(APPEND _ve301_wiringpi_include_hints
            "${WIRINGPI_ROOT}"
            "${WIRINGPI_ROOT}/wiringPi"
        )
        list(APPEND _ve301_wiringpi_library_hints
            "${WIRINGPI_ROOT}"
            "${WIRINGPI_ROOT}/wiringPi"
        )
    endif()

    if(NOT WIRINGPI_INCLUDE_DIR)
        find_path(WIRINGPI_INCLUDE_DIR
            NAMES wiringPi.h
            HINTS ${_ve301_wiringpi_include_hints}
            PATH_SUFFIXES wiringPi
        )
    endif()

    if(NOT WIRINGPI_LIBRARY)
        find_library(WIRINGPI_LIBRARY
            NAMES wiringPi
            HINTS ${_ve301_wiringpi_library_hints}
            PATH_SUFFIXES wiringPi
        )
    endif()

    if(NOT WIRINGPI_INCLUDE_DIR OR NOT WIRINGPI_LIBRARY)
        message(FATAL_ERROR
            "Raspberry builds require WiringPi. Install the WiringPi package in the build environment "
            "so CMake can find wiringPi.h and libwiringPi.")
    endif()
endif()

include(GNUInstallDirs)
include(CheckIPOSupported)

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_GFX REQUIRED IMPORTED_TARGET SDL2_gfx)
pkg_check_modules(MPD REQUIRED IMPORTED_TARGET libmpdclient)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
pkg_check_modules(MNL REQUIRED IMPORTED_TARGET libmnl)

if(WITH_SPOTIFY)
    pkg_check_modules(WEBSOCKETS REQUIRED IMPORTED_TARGET libwebsockets)
endif()

if(WITH_BLUETOOTH)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
endif()

if(WITH_ALSA)
    find_library(ALSA_LIBRARY asound REQUIRED)
endif()

add_library(wifi-scan STATIC
    "${WIFI_SCAN_SOURCE_DIR}/wifi_scan.c"
)

target_include_directories(wifi-scan
    PUBLIC
        "${WIFI_SCAN_SOURCE_DIR}"
)

target_link_libraries(wifi-scan
    PUBLIC
        PkgConfig::MNL
)

set(VE301_SOURCES
    src/base.c
    src/input_menu.c
    src/main.c
    src/radio_app.c
    src/sdl_util.c
    src/theme.c
    src/weather.c
    src/wifi.c
    src/base/config.c
    src/base/log_contexts.c
    src/base/logging.c
    src/base/util.c
    src/menu/glyph_obj.c
    src/menu/menu_ctrl.c
    src/menu/menu_item.c
    src/menu/menu_menu.c
    src/menu/text_obj.c
    src/audio/audio.c
    src/audio/player.c
    src/audio/playlist.c
    src/audio/song.c
    src/audio/radio_browser.c
)

if(WITH_SPOTIFY)
    list(APPEND VE301_SOURCES src/audio/spotify.c)
endif()

if(WITH_BLUETOOTH)
    list(APPEND VE301_SOURCES src/audio/bluetooth.c)
endif()

if(WITH_ALSA)
    list(APPEND VE301_SOURCES src/audio/alsa.c)
endif()

if(VE301_RASPBERRY AND WITH_WIRINGPI)
    list(APPEND VE301_SOURCES src/rotaryencoder.c)
endif()

add_executable(ve301 ${VE301_SOURCES})

target_compile_options(ve301
    PRIVATE
        -Wall
        -fPIC
)

target_compile_definitions(ve301
    PRIVATE
        LOG_LEVEL=${LOG_LEVEL}
)

if(WITH_SPOTIFY)
    target_compile_definitions(ve301 PRIVATE SPOTIFY)
endif()

if(WITH_BLUETOOTH)
    target_compile_definitions(ve301 PRIVATE BLUETOOTH)
endif()

if(VE301_RASPBERRY)
    target_compile_definitions(ve301 PRIVATE RASPBERRY)
    target_compile_options(ve301 PRIVATE -mtune=cortex-a53 -mcpu=cortex-a53)
    target_link_options(ve301 PRIVATE -mtune=cortex-a53 -mcpu=cortex-a53)
endif()

if(VE301_RASPBERRY AND WITH_WIRINGPI)
    target_include_directories(ve301 PRIVATE "${WIRINGPI_INCLUDE_DIR}")
    target_link_libraries(ve301 PRIVATE "${WIRINGPI_LIBRARY}" rt)
endif()

if(FSANITIZE)
    target_compile_options(ve301 PRIVATE -fsanitize=address)
    target_link_options(ve301 PRIVATE -fsanitize=address)
endif()

target_link_libraries(ve301
    PRIVATE
        wifi-scan
        CURL::libcurl
        Threads::Threads
        PkgConfig::SDL2
        PkgConfig::SDL2_IMAGE
        PkgConfig::SDL2_TTF
        PkgConfig::SDL2_GFX
        PkgConfig::MPD
        PkgConfig::CJSON
        m
)

if(WITH_SPOTIFY)
    target_link_libraries(ve301 PRIVATE PkgConfig::WEBSOCKETS)
endif()

if(WITH_BLUETOOTH)
    target_link_libraries(ve301 PRIVATE PkgConfig::DBUS)
endif()

if(WITH_ALSA)
    target_link_libraries(ve301 PRIVATE "${ALSA_LIBRARY}")
endif()

install(TARGETS ve301 RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(EXISTS "${CMAKE_SOURCE_DIR}/src/test_menu.c")
    add_executable(test_menu
        src/test_menu.c
        src/base.c
        src/sdl_util.c
        src/base/config.c
        src/base/log_contexts.c
        src/base/logging.c
        src/base/util.c
        src/menu/glyph_obj.c
        src/menu/menu_ctrl.c
        src/menu/menu_item.c
        src/menu/menu_menu.c
        src/menu/text_obj.c
    )

    target_compile_options(test_menu PRIVATE -Wall -fPIC)
    target_compile_definitions(test_menu PRIVATE MPD_DEBUG LOG_LEVEL=${LOG_LEVEL})

    if(FSANITIZE)
        target_compile_options(test_menu PRIVATE -fsanitize=address)
        target_link_options(test_menu PRIVATE -fsanitize=address)
    endif()

    target_link_libraries(test_menu
        PRIVATE
            CURL::libcurl
            Threads::Threads
            PkgConfig::SDL2
            PkgConfig::SDL2_IMAGE
            PkgConfig::SDL2_TTF
            PkgConfig::SDL2_GFX
            PkgConfig::MPD
            PkgConfig::CJSON
            m
    )

    install(TARGETS test_menu RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
else()
    message(STATUS "Skipping test_menu target because src/test_menu.c is not present.")
endif()
