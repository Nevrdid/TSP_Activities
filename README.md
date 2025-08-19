# Timer

## Usage
- Time a process and add datas to database:
  `activities time <rom_file> <process_pid>`

You can so use it like that for example:
`flycast "$mygame" &`
`activities time "$mygame" $!`

- **NEW**: Watch for file presence and track time:
  `activities -flag <file_path>`

Example:
```bash
# Start watching a file - tracks time while file exists
activities -flag /tmp/cmd_to_run.sh

# The daemon will:
# 1. Start counting time when the file exists
# 2. Stop counting when the file is deleted/moved
# 3. Use inotify for very low CPU usage
# 4. Check every second with minimal overhead
```

**File Watcher Features:**
- Uses `inotify` for efficient file system monitoring
- Very low CPU usage (select() with 1-second timeout)
- Tracks time only while file exists
- Automatically stops when file is removed
- Logs activity to `/mnt/SDCARD/Apps/Activities/log/file_watcher.log`

_____

# GUI

## Usage

- Start gui in list window: `activities gui`
- Start gui in a game details: `activities gui <rom_file>`
  (If the game is not in the database, it will be added)

## Config
Edit the config.ini file for:
- Set target device
    `device=tsp/brick`
- Set theme to grab systems background from
    `background_theme=Burst!/CrossMix - OS/TRIMUI Blue...`
- Set theme to grab skin from:
    `skin_theme=Default/Burst!/Greys-Dark...`
- Set primary/secondary colors ( should be removed to use themes ones soon)
    `primary_color=blue,red,lightgreen,black`
    `secondary_color=...`


## Keybinds

### List window

- A: Open game details
- B: Exit

- Up/Down: Navigate one by one in list
- Left/Right: Navigate ten by ten in list

- L1/R1: Filter systems
- Select: Filter (un)completed/All
- Start: Change sorting method
- Menu: Open overall stats

### Details window

- A: Launch game
- B: Return to list

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

## Cross-compilation for TrimUI TSP

### Prerequisites

You need the aarch64 cross-compilation toolchain. Based on the example provided:

```bash
# Toolchain path
/opt/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-gcc

# Sysroot 
/opt/aarch64-linux-gnu-7.5.0-linaro/sysroot

# TSP specific libraries
/root/workspace/minui-presenter/platform/tg5040/lib/
```

### Compilation

1. **Test your environment:**
   ```bash
   chmod +x test_crosscompile.sh
   ./test_crosscompile.sh
   ```

2. **Compile for TrimUI TSP:**
   ```bash
   # Using Makefile
   make tsp
   
   # Or using the build script
   chmod +x build_tsp.sh
   ./build_tsp.sh
   ```

3. **Compile for testing (native):**
   ```bash
   make test
   ```

### Makefile Features

- **Cross-compilation support**: Automatically uses aarch64 toolchain when not in TEST mode
- **Optimized for TSP**: Uses `-O3 -Ofast -flto` for maximum performance
- **Proper linking**: Links against TSP-specific SDL2, sqlite3, and system libraries
- **Sysroot support**: Uses proper cross-compilation sysroot
- **Debug info**: `make info` shows compilation variables

### File Structure

```
activities              # Final binary for TrimUI TSP
activities.x86          # Test binary for x86 systems
build/                  # Object files directory
build_tsp.sh           # Cross-compilation script
test_crosscompile.sh   # Environment test script
```

# Timer

## Usage
- Time a process and add datas to database:
  `activities add <rom_file> <process_pid>`

You can so use it like that for example:
`flycast "$mygame" &`
`activities "$mygame" $!`

- **NEW**: Watch for file presence and track time:
  `activities -flag <file_path>`

Example:
```bash
# Start watching a file - tracks time while file exists
activities -flag /tmp/cmd_to_run.sh

# The daemon will:
# 1. Start counting time when the file exists
# 2. Stop counting when the file is deleted/moved
# 3. Use inotify for very low CPU usage
# 4. Check every second with minimal overhead
```

**File Watcher Features:**
- Uses `inotify` for efficient file system monitoring
- Very low CPU usage (select() with 1-second timeout)
- Tracks time only while file exists
- Automatically stops when file is removed
- Logs activity to `/mnt/SDCARD/Apps/Activities/log/file_watcher.log`

## Activity Tracking

The `activities` command can be used to track time for specific activities, either by monitoring a process or a file.

### Process Monitoring

To track the time of a running process, use the command:

```bash
activities time "<activity_name>" <process_pid>
```

Example:
```bash
activities time "My Awesome Game" 12345
```

This will start a timer for "My Awesome Game" and will stop when the process with PID 12345 terminates.

### File Watcher Mode

Alternatively, you can track an activity by monitoring a file. The timer will stop as soon as the specified file is deleted. This is useful for scripts or tasks that signal their completion by removing a temporary file.

Example:
```bash
activities time "My Script Task" -flag /tmp/my_script.lock
```

This command starts a timer for "My Script Task" and watches for the deletion of `/tmp/my_script.lock`.

## GUI

To launch the graphical interface, simply run:
```bash
activities gui
```

# Issue tracker:
    - Setting a running game to complete overwrite the green dot but running a completed work as expected.

# Improvements possibilities:
    - Allow to merge multiple games datas ( e.g: file renamed/moved)
    - Stay on same game if possible when filtering
    - 
