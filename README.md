# Timer

## Usage
- Time a process and add datas to database:
  `activities time <rom_file> <process_pid>`

You can so use it like that for example:
`flycast "$mygame" &`
`activities time "$mygame" $!`

_____

# GUI

## Usage

- Start the GUI on the home page: `activities gui`
- Start directly in a game details view: `activities gui <rom_file>`
  (If the game is not in the database, it will be added)

The home page links to the game library, recent games, All ROMs, overall statistics, and Add Game. Recent games shows titles with at least one recorded play session. All ROMs scans files beneath the portable `Roms/` directory (or `/mnt/SDCARD/Roms` on TrimUI); selecting a file adds it to the database if needed and opens its details. The library and details views remain available from the existing controller navigation.

## Theming

On handhelds, the app follows the system theme configuration. The desktop bundle includes minimal TRIMUI Blue and TRIMUI Retro theme assets used by this app.

## Keybinds

### Home page

- A: Open the selected section
- B: Exit
- Up/Down: Navigate

### All ROMs

- A: Open the selected ROM (adds it to the database if needed)
- B: Return to home
- Up/Down: Navigate

### List window

- A: Open game details
- B: Return to home page

- Up/Down: Navigate one by one in list
- Left/Right: Navigate ten by ten in list

- L1/R1: Filter systems
- Select: Filter (un)completed/All
- Start: Change sorting method
- Menu: Open overall stats

### Details window

- A: Launch game
- B: Return to home page
- X: Return to list

- Y: Start game's video (/mnt/SDCARD/Videos/[System]/[gamename].mp4
- X: Start game's manual (upcomming)

- Select: Set game as completed
- Menu: Remove the game from the database.

## Licenses

### MIT License for `nlohmann/json`

This project uses the `nlohmann/json` library, which is licensed under the MIT License. The full license text is as follows:
```
MIT License

Copyright © 2013-2024 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

# Issue tracker:
    - Setting a running game to complete overwrite the green dot but running a completed work as expected.

# Improvements possibilities:
    - Stay on same game if possible when filtering

## Native Linux x86-64 build

The GitHub Actions workflow **Native x86-64 build** publishes two artifacts:

- `tsp-activities-linux-x86_64`: the standalone GUI and timer binaries.
- `tsp-activities-linux-x86_64-portable`: a portable tarball with the binaries, app assets, minimal theme configuration/font/UI assets, and ready-to-populate `Roms/`, `Imgs/`, and `Emus/` directories.

Extract the portable archive and run `./activities.x86` directly. The binary resolves assets, themes, emulator configuration, and the database relative to its own location, so it does not depend on the terminal's current working directory. The app creates `data/` and `data/games.db` on first launch.

The native build requires a 64-bit Linux system with SDL2, SDL2_ttf, SDL2_image, and SQLite runtime libraries installed.
