# AGENTS.md

This directory contains the menu and rendering implementation for VE301. The primary path is the C implementation in `menu_ctrl.c`, `menu_menu.c`, `menu_item.c`, `glyph_obj.c`, and `text_obj.c`. The `Menu.cpp`, `MenuCtrl.cpp`, and `MenuItem.cpp` files are thin C++ wrappers around the same C API and are built separately through the `mainObjective` target.

## What Matters Most

- Changes here usually affect SDL2/TTF rendering and the menu navigation behavior directly.
- Keep changes narrow. The menu is stateful and visually coupled; small edits are much safer than refactors.
- Be very careful with ownership. Many objects are managed manually with `malloc`, `calloc`, `free`, `SDL_FreeSurface`, `SDL_DestroyTexture`, and `TTF_CloseFont`.
- The C API is the source of truth. Only touch the C++ files when the wrapper surface or `mainObjective` is actually involved.
- Do not assume threading. The code is effectively single-threaded and uses global SDL/TTF state.

## Code Map

- `menu_ctrl.c`
  Orchestrates the window, renderer, input, themes, transitions, and the active menu tree.
- `menu_menu.c`
  Manages the menu itself: items, rotation, drawing, dirty state, and submenus.
- `menu_item.c`
  Encapsulates item logic, actions, visibility, labels, and submenus.
- `glyph_obj.c` and `text_obj.c`
  Create and draw glyph and text objects, including bumpmap and shading effects.
- `menu_*_priv.h`
  Contain internal structures and should only change when the implementation truly needs new fields.
- `Menu*.cpp`
  Wrapper layer around the C API. If you change them, keep the public C API aligned as well.

## Typical Pitfalls

- Do not forget `dirty` flags: after state changes that affect rendering, the affected menu must be marked for redraw.
- Index and wraparound logic is sensitive. `current_id`, `active_id`, `max_id`, `segment`, and `n_o_items_on_scale` are tightly coupled.
- Submenus and transitions use `menu_fade_in`/`menu_fade_out` and set `ctrl->current`/`ctrl->active`. Do not change those sequences casually.
- Text and glyph objects are often rebuilt. Make sure old objects are freed before replacing them.
- The `icon` path and multiline labels share parts of the text logic. Changes to `text_obj_new` must handle both cases.
- SDL rendering follows concrete renderer, texture, and surface lifecycles. An object must never be freed twice.

## Build and Validation

Preferred checks for changes in this directory:

- Desktop build: `make -C PC`
- Desktop build with AddressSanitizer: `make -C PC FSANITIZE=1`
- Menu test target: `make -C PC test_menu`
- C++ wrapper target: `make -C PC mainObjective`

If you only change the C implementation, `make -C PC` or `make -C PC test_menu` is usually enough. If you touch `Menu*.cpp` or their headers, also check `make -C PC mainObjective`.

## Change Rules

- Use the existing C style: simple functions, explicit ownership rules, little abstraction.
- If you change API signatures, check all call sites in the menu tree and the wrappers.
- Do not make large restructurings without a clear reason.
- Do not add or modify generated or built artifacts here.
- For UI or layout changes, always consider drawing and state transitions together.
