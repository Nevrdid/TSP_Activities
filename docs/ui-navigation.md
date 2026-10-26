# UI navigation plan

## Goals

- Give the application a clear landing page when launched interactively.
- Keep the existing game list and game details view intact while navigation is introduced.
- Reuse the current SDL renderer, theme colors, and theme assets instead of importing the old MyUI page framework.
- Keep navigation state in the application layer; keep rendering primitives and theme handling in `GUI`.
- Preserve command-line entry points so explicit CLI invocations can continue opening the requested view.

## Initial page map

```text
Home
 ├─ Game library ── Game details ── back to library
 ├─ Recent games ── Game details ── back to library
 ├─ All ROMs ── open / track a file ── Game details
 ├─ Overall statistics ── back to home
 └─ Add game ── back to home

B exits from Home
```

This is intentionally small. Recent games reuses the current library renderer and filters to games with at least one recorded session. The All ROMs browser scans regular files beneath the portable `Roms/` folder, or `/mnt/SDCARD/Roms` when available. Hidden files and directories are skipped. Selecting a file adds it to the database if needed, then opens its details. Collections and settings remain out of scope until their behavior is defined.

## Responsibilities

- `Activities`: owns the current page and transitions between pages; calls existing game/database actions.
- `GUI`: owns SDL initialization, input mapping, rendering helpers, cached resources, and theme integration.
- `DB` / `Rom`: remain the source of persisted game and activity data.

The first implementation should not introduce a generic page inheritance framework or make every page detail configurable. A small page enum and focused render/input methods are sufficient for the current application.

## Incremental rollout

1. Introduce a home page and explicit Home / Library / Details / All ROMs navigation state.
2. Keep existing library filtering, game details, database initialization, and portable path behavior unchanged.
3. Adapt visual ideas from the archived MyUI prototype using current theme resources and the current codebase.
4. Extend the all-ROMs browser with system-aware filtering and file-type rules only when the available emulator metadata supports them reliably.
