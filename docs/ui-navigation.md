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
 ├─ Library ── browse files under Roms/ ── open / track a ROM
 ├─ Recent games ── tracked games with play history ── Game details
 ├─ Overall statistics
 └─ Browse files / Add game ── choose a ROM outside the normal library
```

The Library is the filesystem view of games available under the portable `Roms/` folder, or `/mnt/SDCARD/Roms` on TrimUI. It is not a database-only list: ROMs are visible even before they have been tracked. Opening a ROM creates a database entry if needed and opens its details. Recent games remains a database-backed view filtered to titles with at least one recorded session. Browse files / Add game is a separate file-selector action for importing a ROM from another location. There is no separate top-level All ROMs page because it duplicates the Library.

Collections and settings remain out of scope until their behavior is defined.

## Responsibilities

- `Activities`: owns the current page and transitions between pages; calls existing game/database actions.
- `GUI`: owns SDL initialization, input mapping, rendering helpers, cached resources, and theme integration.
- `DB` / `Rom`: remain the source of persisted game and activity data.

The first implementation should not introduce a generic page inheritance framework or make every page detail configurable. A small page enum and focused render/input methods are sufficient for the current application.

## Incremental rollout

1. Introduce a home page and explicit Home / filesystem Library / tracked Recent games / Details navigation state.
2. Keep existing library filtering, game details, database initialization, and portable path behavior unchanged.
3. Adapt visual ideas from the archived MyUI prototype using current theme resources and the current codebase.
4. Extend the all-ROMs browser with system-aware filtering and file-type rules only when the available emulator metadata supports them reliably.
