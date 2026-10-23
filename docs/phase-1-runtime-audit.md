# Phase 1 runtime and portability audit

This document records observations from the initial source review. It is a map of current behavior, not a promise that every listed path is required on every target. Validate on the intended handheld before changing platform behavior.

## Build baseline

- The native x86-64 workflow is `.github/workflows/build-native.yml`; it builds `activities.x86` and `timer.x86` on Ubuntu 24.04.
- The Makefile now creates each object and binary output directory as part of the relevant rule. A clean checkout should not need CI-specific directory preparation.
- The handheld ARM64/release workflow uses a Debian Bullseye ARM64 container. Its base is intentional for compatibility with older handheld runtime libraries; do not upgrade it casually. Since Bullseye security packages have moved to the Debian archive, the workflow now redirects the security repository there while retaining Bullseye.
- `build/config_native_aarch64.mk` now configures a native ARM64 build in the Bullseye container. The workflow builds `build/bin/activities`, packages the tracked `.assets/` directory, and uploads an ARM64 archive for branch verification. This is not yet proof of runtime compatibility on the physical handheld.
- A successful compile does not prove that the UI can launch or that device assets/configuration are available.

## Runtime dependencies found

### Configuration and theme

`srcs/activities/Config.cpp` reads `/mnt/UDISK/system.json` for the `theme` path and optionally reads `/mnt/SDCARD/System/etc/crossmix.json` for `BACKGROUNDS`. The sample files supplied during the audit confirm that the theme config contains `font`, `fontascii`, `fontsize`, and `fontcolor` fields. The special system-theme value `../res/` is still rewritten to `/mnt/SDCARD/Themes/CrossMix - OS/`.

The constructor now reports malformed system JSON clearly, retains built-in colors when theme color fields are missing, treats CrossMix's configuration file as optional (defaulting the background selection to `Default`), and normalizes the configured theme path to end in `/` so asset lookups do not depend on the JSON value including a trailing separator. This preserves the existing CrossMix values when present while removing an unnecessary hard dependency on that one file. The system JSON and usable theme/font/assets are still required by the current startup path; this is not yet a complete standalone configuration system.

Implications:
- The configured theme path remains tied to the handheld filesystem layout unless the later platform/path abstraction changes it.
- The theme's `config.json` is a style schema, not a full app configuration. It does not define ROM roots, database location, or emulator launchers.
- Missing theme/font/skin files can still leave the UI incomplete. Do not replace these paths until the needed assets and fallback behavior have been verified.


### Theme asset inventory

The current code directly references these assets under `<theme_path>/skin/`:

- `bg.png` — default background.
- `title-bg.png`, `tips-bar-bg.png`, `list-item-1line-sort-bg-f.png`, `list-item-1line-sort-bg-n.png`, `list-item-2line-long-bg-n.png`.
- `icon-star.png`, `ic-game-580.png`, `bg-menu-09.png`, `bg-menu-05.png`, `ic-keymap-n.png`, `ic-right-arrow-n.png`, `ic-left-arrow-n.png`.
- `float-win-mask.png`, `background-mask.png`, `pop-bg.png`, `btn-bg-f.png`, and `btn-bg-n.png`.
- Button-hint icons from `includes/GUI.hpp`: `tips-A.png`, `tips-B.png`, `button-tips-X.png` and the other names in the `buttons_icons` map.

The UI also opens `<theme_path>/<font>` using SDL_ttf. The theme parser reads `fontascii` and `fontsize`, but the current GUI code does not use those values; font opening uses only `theme.font` and a size derived from the UI's own constants. This is a code-level mismatch to address separately from asset bundling.

The CrossMix-OS repository contains a `Themes/CrossMix - OS` theme with a `skin/` directory and `wqy-microhei.ttf`, so it is a useful compatibility reference. It should not be copied wholesale into this app as a shortcut: the theme contains many unrelated files, and font/asset redistribution should be checked at the file/license level. The upstream repository has a GPL-3.0 license file, but that alone does not establish the redistribution terms for every bundled third-party font or image. In particular, do not redistribute `wqy-microhei.ttf` or theme artwork until their own licensing and attribution requirements are verified. Prefer newly created app-owned fallback graphics and a font with an explicit compatible license. The immediate standalone-work plan is to define an app-owned asset root and a small, explicitly licensed fallback theme, then keep external theme selection as an optional override. Until that exists, a successful build does not mean the package can run without CrossMix theme files.

### UI, SDL and input

`srcs/activities/GUI.cpp` initializes SDL video and joystick support, opens joystick 0 when available, creates a 320x200 fullscreen window, requests an accelerated vsynced renderer, and initializes SDL_ttf. It also includes Linux framebuffer and memory-mapping headers. Fonts are opened from the configured theme path.

Implications:
- Runtime behavior depends on SDL2, SDL2_ttf, a working video backend/renderer, and device input behavior.
- Missing joystick is logged but is not itself fatal; failure to initialize SDL or create the window/renderer is a problem.
- Missing fonts or theme assets can prevent text from rendering correctly.

### ROM assets and launchers

`srcs/utils.cpp` also contains process helpers and directory listing code. The process lookup now closes its `popen()` stream with `pclose()` (rather than `fclose()`) and treats a failed PID parse as no result. Directory filtering now checks the filename string safely before testing its first character.

`srcs/activities/Rom.cpp` derives image, video and manual paths from ROM paths under `/Roms/<system>/...` or the alternate `/Best/<system>/...` layout. Game launching invokes `/mnt/SDCARD/Emus/<system>/default.sh`. The native build configuration points video/manual actions at desktop commands, while the handheld cross configuration points at handheld scripts.

Implications:
- Asset discovery and game launching are platform-specific and currently use fixed paths.
- Keep native and handheld launch behavior separate until there is an explicit platform/path abstraction.
- Verify child-process creation and launcher failure handling as a dedicated task.

### Persistence and packaging

The application links SQLite. The release workflow packages `icon.png`, `launch.sh`, `config.json`, and the tracked `.assets/` directory. `data/config.ini` is not tracked and its `CONFIG_FILE` macro has no current source-code consumer, so packaging now includes it only if a deployment supplies it. The runtime configuration still comes from the two absolute-path JSON files read by `Config.cpp`.

The database path is defined as `APP_DIR "data/games.db"` in `includes/DB.hpp`. `DB.cpp` opens that path and creates the table if needed. The ARM64 packaging workflow creates the parent `data/` directory, but a manual install must do so too. The constructor now stops initialization and closes the failed SQLite handle when opening the database fails; `is_refresh_needed()`, both `load()` overloads, `remove()`, and now `save()` check for a missing connection before issuing SQLite calls. This prevents a failed database open from turning a later save into a null-connection call. The application still logs and skips persistence if the database cannot be opened; creating the directory at runtime remains a possible future hardening step.

## Fixes made during the audit

- ROM sort comparators now compare in the requested direction instead of negating the ascending comparison. Negation incorrectly returns true when values are equal, violating the strict-weak-ordering requirement of `std::sort`. The time sort now compares numeric `totaltime` values instead of formatted display strings.
- Image loading checks `IMG_Load` and texture creation before accessing surface dimensions. Failed assets are logged and skipped rather than dereferenced.
- Text rendering now checks font lookup, surface rendering and texture creation before accessing the surface. It no longer closes a cached font when rendering fails.
- SDL cleanup now tolerates a missing joystick and partially initialized renderer/window/font resources. The Activities startup path stops cleanly if GUI initialization fails.
- The completed-game icon path now matches the tracked `.assets/green-check.svg` filename.
- `Rom::get()` now handles an empty ROM list safely; the old `do/while` dereferenced `end()` before checking whether the list was empty.

Configuration hardening now also catches malformed system/theme/CrossMix JSON and preserves built-in colors when optional theme colors are absent. CrossMix background settings are optional, defaulting to `Default`; the required handheld `system.json` behavior remains unchanged. The ARM64 config's `APP_DIR` macro quoting was corrected after the first Bullseye build exposed compiler errors; rerun the ARM64 workflow before treating the fix as verified.

These changes preserve the existing C++17 baseline and do not introduce newer runtime APIs.

## Follow-up order

1. Keep native CI green and make Makefile output-directory handling self-contained.
2. Trace database initialization and document the actual DB path and schema lifecycle.
3. Add safe checks for missing theme/font/image assets and test behavior on-device.
4. Audit child process and signal handling around launch, suspend, resume and stop.
5. Design a small platform configuration/path layer, preserving the existing handheld defaults.
6. Repair and verify the ARM64 packaging workflow against the existing Bullseye-compatible environment before changing its compiler or library baseline.

## Compatibility rule

Do not raise the C++ language level, libc/libstdc++ baseline, SDL minimum version, or other runtime requirements as a side effect of cleanup. Test changes in native CI, and separately verify ARM64 behavior in the established handheld build environment.
