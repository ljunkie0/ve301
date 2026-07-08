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
option(WITH_MENU_WEB "Build Mongoose-backed menu web service" OFF)
option(FSANITIZE "Enable AddressSanitizer" OFF)
option(VE301_RASPBERRY "Enable Raspberry Pi specific sources and flags" OFF)
option(WITH_WIRINGPI "Build WiringPi-backed rotary encoder support" OFF)

set(LOG_LEVEL "5" CACHE STRING "Compile-time log level")
set(WIFI_SCAN_SOURCE_DIR "" CACHE PATH "Path to a wifi-scan checkout")
set(WIRINGPI_ROOT "" CACHE PATH "Path to the WiringPi project root")
set(WIRINGPI_INCLUDE_DIR "" CACHE PATH "Path to the WiringPi include directory")
set(WIRINGPI_LIBRARY "" CACHE FILEPATH "Path to the WiringPi library")
set(MONGOOSE_SOURCE_DIR "" CACHE PATH "Path to Mongoose source directory")

if(WITH_MENU_WEB AND NOT MONGOOSE_SOURCE_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/PC/mongoose/mongoose.c")
        set(MONGOOSE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/PC/mongoose")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/Raspberry/mongoose/mongoose.c")
        set(MONGOOSE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Raspberry/mongoose")
    endif()
endif()

if(WITH_MENU_WEB AND NOT EXISTS "${MONGOOSE_SOURCE_DIR}/mongoose.c")
    message(FATAL_ERROR
        "WITH_MENU_WEB requires Mongoose sources. Run make -C PC WITH_MENU_WEB=1 or set MONGOOSE_SOURCE_DIR to a directory containing mongoose.c and mongoose.h.")
endif()

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
pkg_check_modules(XML2 REQUIRED IMPORTED_TARGET libxml-2.0)
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
    src/base/base.c
    src/input_menu/input_menu.c
    src/main.c
    src/radio_browser/menu.c
    src/podcast/menu.c
    src/podcast/podcast.c
    src/radio_app/actions.c
    src/radio_app/config.c
    src/radio_app/core.c
    src/radio_app/info_menu.c
    src/radio_app/navigation_menu.c
    src/radio_app/navigation_hooks.c
    src/radio_app/network_menu.c
    src/radio_app/players.c
    src/radio_app/themes.c
    src/radio_app/volume_menu.c
    src/util/sdl_util.c
    src/radio_app/theme.c
    src/weather/weather.c
    src/util/wifi.c
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
    src/audio/mpd_media_player.c
    src/audio/player.c
    src/audio/playlist.c
    src/audio/song.c
    src/radio_browser/radio_browser.c
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

if(WITH_MENU_WEB)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(MENU_WEB_INDEX_HTML "${CMAKE_SOURCE_DIR}/src/menu/web/index.html")
    set(MENU_WEB_INDEX_HTML_INC "${CMAKE_CURRENT_BINARY_DIR}/menu/web/index_html.inc")
    add_custom_command(
        OUTPUT "${MENU_WEB_INDEX_HTML_INC}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/menu/web"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/embed_text.py" "${MENU_WEB_INDEX_HTML}" "${MENU_WEB_INDEX_HTML_INC}"
        DEPENDS "${MENU_WEB_INDEX_HTML}" "${CMAKE_SOURCE_DIR}/tools/embed_text.py"
        VERBATIM
    )
    list(APPEND VE301_SOURCES
        src/menu/web/menu_web.c
        "${MENU_WEB_INDEX_HTML_INC}"
        "${MONGOOSE_SOURCE_DIR}/mongoose.c"
    )
endif()

if(VE301_RASPBERRY AND WITH_WIRINGPI)
    list(APPEND VE301_SOURCES src/raspberry/rotaryencoder.c)
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

if(WITH_MENU_WEB)
    target_compile_definitions(ve301 PRIVATE MENU_WEB)
    target_include_directories(ve301 PRIVATE "${MONGOOSE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/menu/web")
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
        PkgConfig::XML2
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
        src/base/base.c
        src/util/sdl_util.c
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
