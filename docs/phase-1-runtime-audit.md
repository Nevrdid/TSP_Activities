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

`srcs/activities/Config.cpp` opens both `/mnt/UDISK/system.json` and `/mnt/SDCARD/System/etc/crossmix.json`. If either file cannot be opened, construction logs an error and exits. The first file provides a theme path; the special value `../res/` is rewritten to the CrossMix theme directory. A theme config is loaded from `<theme_path>/config.json` when it exists.

Implications:
- The current startup path is tied to the expected handheld/CrossMix filesystem layout.
- Theme fields are used to populate colors, but behavior when the theme config is absent or incomplete needs validation.
- Before replacing these dependencies, define a fallback configuration format and retain the existing device behavior.

### UI, SDL and input

`srcs/activities/GUI.cpp` initializes SDL video and joystick support, opens joystick 0 when available, creates a 320x200 fullscreen window, requests an accelerated vsynced renderer, and initializes SDL_ttf. It also includes Linux framebuffer and memory-mapping headers. Fonts are opened from the configured theme path.

Implications:
- Runtime behavior depends on SDL2, SDL2_ttf, a working video backend/renderer, and device input behavior.
- Missing joystick is logged but is not itself fatal; failure to initialize SDL or create the window/renderer is a problem.
- Missing fonts or theme assets can prevent text from rendering correctly.

### ROM assets and launchers

`srcs/activities/Rom.cpp` derives image, video and manual paths from ROM paths under `/Roms/<system>/...` or the alternate `/Best/<system>/...` layout. Game launching invokes `/mnt/SDCARD/Emus/<system>/default.sh`. The native build configuration points video/manual actions at desktop commands, while the handheld cross configuration points at handheld scripts.

Implications:
- Asset discovery and game launching are platform-specific and currently use fixed paths.
- Keep native and handheld launch behavior separate until there is an explicit platform/path abstraction.
- Verify child-process creation and launcher failure handling as a dedicated task.

### Persistence and packaging

The application links SQLite. The release workflow packages `icon.png`, `launch.sh`, `config.json`, and the tracked `.assets/` directory. `data/config.ini` is not tracked and its `CONFIG_FILE` macro has no current source-code consumer, so packaging now includes it only if a deployment supplies it. The runtime configuration still comes from the two absolute-path JSON files read by `Config.cpp`.

The database path is defined as `APP_DIR "data/games.db"` in `includes/DB.hpp`. `DB.cpp` opens that path and creates the table if needed, but does not create the parent `data/` directory first. If the directory is missing or unwritable, `sqlite3_open` can fail; the constructor logs the error but then proceeds to execute the table-creation query. The constructor now stops initialization and closes the failed SQLite handle when opening the database fails; `is_refresh_needed()` also checks for a missing connection. This avoids continuing into table creation with an invalid connection, but does not create the parent directory automatically. Verify whether the packaging/runtime always creates `data/` and whether the current device install has existing database data before altering behavior.

## Fixes made during the audit

- ROM sort comparators now compare in the requested direction instead of negating the ascending comparison. Negation incorrectly returns true when values are equal, violating the strict-weak-ordering requirement of `std::sort`. The time sort now compares numeric `totaltime` values instead of formatted display strings.
- Image loading checks `IMG_Load` and texture creation before accessing surface dimensions. Failed assets are logged and skipped rather than dereferenced.
- Text rendering now checks font lookup, surface rendering and texture creation before accessing the surface. It no longer closes a cached font when rendering fails.
- SDL cleanup now tolerates a missing joystick and partially initialized renderer/window/font resources. The Activities startup path stops cleanly if GUI initialization fails.
- The completed-game icon path now matches the tracked `.assets/green-check.svg` filename.

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
