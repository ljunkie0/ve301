# AGENTS.md

This repository is a small SDL2-based internet radio application written mostly in C, with a small parallel C++ menu wrapper layer in `src/menu/cpp/` and `src/mainObjective.cpp`.
The production app entrypoint is `src/main.c`; most runtime behavior is orchestrated from `src/radio_app.c`.

## What Matters Most

- Prefer editing `src/`. `PC/` and `Raspberry/` are primarily build directories and may contain generated artifacts, test outputs, Docker assets, or cached third-party code.
- Treat the Makefile build as the canonical path. CMake exists and is usable, but the Makefiles still encode the repo's original workflow and some platform assumptions.
- Do not assume a clean worktree. This repo often contains user edits and build outputs mixed together.
- Keep changes narrow. The codebase is stateful, manually managed, and not heavily test-covered.
- If a change touches menu rendering, wrapper bindings, or C++ entrypoints, also check the dedicated menu guidance in [`src/menu/AGENTS.md`](/home/chris/Dokumente/Development/ve301/src/menu/AGENTS.md).

## Code Map

- `src/main.c`
  Starts the app with `radio_app_init()`, `radio_app_loop()`, and `radio_app_close()`.
- `src/radio_app.c`
  Central coordinator. Builds menus, reads config, applies themes, manages active players, updates weather/time/info views, and bridges UI to audio/network state.
- `src/audio/audio.c`
  MPD integration. Handles connection management, queue/playback/volume, and internet radio playlist access.
- `src/audio/player.c`
  Shared player state abstraction used by radio, Bluetooth, Spotify, and ALSA-backed sources.
- `src/audio/spotify.c`
  Spotify websocket integration plus cover-art download into `/tmp`.
- `src/audio/bluetooth.c`
  DBus listener for BlueALSA/BlueZ events; updates the shared player metadata.
- `src/weather.c`
  Curl + cJSON weather fetcher with its own thread/timer state.
- `src/menu/*.c`
  Custom circular menu system, SDL rendering, transitions, and menu item behavior. This code is performance-sensitive and visually coupled.
- `src/menu/cpp/*.cpp`
  Thin C++ wrappers around the menu C API. They should stay aligned with the underlying C headers and constructors.
- `src/base/*.c`
  Config parsing, logging, utility helpers, and shared low-level support.
- `src/wifi.c`
  Wrapper around the external `wifi-scan` source tree.
- `src/rotaryencoder.c`
  Raspberry-only physical input support.
- `playground/`
  Experiments only; not part of the main app path.

## Build Paths

### Makefile builds

Desktop:

```bash
make -C PC
```

Desktop with ASan:

```bash
make -C PC FSANITIZE=1
```

Raspberry Docker flow:

```bash
make -C Raspberry armhf-docker
```

Notes:

- Root `Makefile` contains the real source/object rules.
- `PC/Makefile` defaults to `WITH_SPOTIFY=1` and `WITH_BLUETOOTH=0`.
- `Raspberry/Makefile` defaults to `WITH_SPOTIFY=1` and `WITH_BLUETOOTH=1`.
- Raspberry builds add `rotaryencoder.o`, WiringPi, and `-DRASPBERRY`.
- The build may clone `wifi-scan` if its sources are missing.
- `make clean` does not necessarily remove every generated file, so stale objects can survive if the full build graph is not exercised.

### CMake builds

CMake is present and useful, especially for desktop or cross-build setup:

```bash
cmake --preset desktop
cmake --build --preset desktop
```

Presets currently exist for:

- `desktop`
- `desktop-asan`
- `raspberry`

Prefer Makefiles if you need to match existing behavior exactly. Use CMake when the task is clearly about the newer build system.

## Runtime Assumptions

- Runtime config is expected at `~/.ve301/config`.
- `sample.config` is the reference for supported keys.
- MPD defaults to `localhost:6600`.
- The radio UI expects an MPD playlist to populate station/menu content.
- Weather requires an OpenWeather API key and location settings.
- Spotify, Bluetooth, and ALSA behavior are compile-time optional and partly config-dependent.

## Editing Guidance

- Preserve the existing C style. This code uses straightforward functions, manual memory management, and light abstraction.
- Be careful with ownership. Many setters duplicate strings, but not all call sites are obviously symmetric.
- Be careful with threads and globals. MPD, weather, and Spotify all maintain process-global state.
- Avoid large refactors unless the task explicitly requires one. The app logic is centralized and side effects are easy to break.
- When changing UI/menu behavior, inspect both drawing code and the state transitions that trigger redraws.
- When changing config behavior, update `sample.config` if the visible surface area changes.
- Do not edit generated or downloaded artifacts unless the task is explicitly about build tooling.

## Validation

Prefer the smallest relevant validation step:

- Build desktop app: `make -C PC`
- Build desktop app with sanitizers: `make -C PC FSANITIZE=1`
- Build menu test binary if available: `make -C PC test_menu`
- Build C++ wrapper target: `make -C PC mainObjective`
- Build CMake desktop preset: `cmake --build --preset desktop`

If a task touches Raspberry-only code, validate with the Raspberry build path when practical. If that is not practical, say so explicitly.

## Repo-Specific Pitfalls

- `PC/` and `Raspberry/` can contain both sources-of-truth Makefiles and non-source artifacts. Check before editing.
- `wifi-scan` is external code pulled into the build; prefer adapting the wrapper/integration rather than modifying vendored content.
- Some files in this repo are already mid-edit. Read the current file content carefully and avoid reverting unrelated user work.
- Optional features are controlled by compile definitions such as `SPOTIFY`, `BLUETOOTH`, `ALSA`, and `RASPBERRY`; changes may need to compile in multiple flag combinations.
- Wrapper-layer changes can affect both the C API and the C++ entrypoint, so validate both paths when possible.
