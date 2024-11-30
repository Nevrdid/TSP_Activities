
# Timer

## Usage
- Time a process and add datas to database:
  `activities add <rom_file> <process_pid>`

You can so use it like that for example:
`flycast "$mygame" &`
`activities "$mygame" $!`

# GUI

## Usage

- Start gui in list window: `activities gui`
- Start gui in a game details: `activities gui <rom_file>`
  (If the game is not in the database, it will be added)

## Config
Edit the activities.cfg for gui design settings.
- default_background (string) - background image to use if not using theme or if a theme image is missing.
- theme_background (bool) - use system's background per system instead of default on
- theme_name (string) - select the theme to get images from

## Keybinds

### List window

- A: Open game details
- B: Exit

- Up/Down: Navigate one by one in list
- Left/Right: Navigate ten by ten in list

- L1/R1: Filter systems
- Select: Filter (un)completed/All
- Start: Change sorting method

### Details window

- A: Launch game
- B: Return to list
- Y: Start game's video (/mnt/SDCARD/Videos/[System]/[gamename].mp4
- X: Start game's manual (upcomming)
- Select: Set game as completed


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
