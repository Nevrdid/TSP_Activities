
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
