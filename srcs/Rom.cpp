#include "Rom.h"

#include "utils.h"

#include <fstream>
#include <iostream>
#include <regex>

static std::regex img_pattern = std::regex(R"(\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"
static std::regex sys_pattern =
    std::regex(R"(.*\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"
// Matches "/Best/<subfolder>" (for alternate library root)
static std::regex               best_pattern = std::regex(R"(\/Best\/([^\/]+).*)");
std::vector<Rom>                Rom::list;
std::unordered_set<std::string> Rom::ra_hotkey_roms;
std::unordered_set<std::string> Rom::childs;
GUI&                            Rom::gui = GUI::getInstance();
Config&                         Rom::cfg = Config::getInstance();
DB&                             Rom::db = DB::getInstance();

std::vector<Rom>& Rom::get()
{
    return list;
}

Rom* Rom::get(const std::string& rom_file)
{
    auto it = list.begin();
    do {
        if (it->file == rom_file || fs::path(it->file).filename() == fs::path(rom_file)) {
            std::cout << "ROM " << it->name << "found in database." << std::endl;

            return &(*it);
        }
    } while (++it != list.end());
    return nullptr;
}

void Rom::refresh()
{
    std::vector<Rom> previous_roms = list;
    list.clear();
    std::vector<DB_row> table = db.load();
    for (DB_row row : table) {
        std::vector<Rom>::iterator previous_rom = std::find_if(
            previous_roms.begin(), previous_roms.end(), [&](Rom& r) { return r.file == row.file; });
        if (previous_rom != previous_roms.end()) {
            previous_rom->update(row);
            list.push_back(*previous_rom);
        } else {
            Rom rom(row);
            list.push_back(rom);
        }
    }

    std::cout << "Loaded " << list.size() << " roms." << std::endl;
    for (const auto& rom : list)
        std::cout << "  - " << rom.name << " (" << rom.file << ")" << std::endl;
}

DB_row Rom::get_DB_row()
{
    return {file, name, count, time, lastsessiontime, last, completed, favorite};
}

void Rom::update(DB_row row)
{
    count = row.count;
    time = row.time;
    lastsessiontime = row.lastsessiontime;
    last = row.last;
    total_time = utils::stringifyTime(time);
    average_time = utils::stringifyTime(count ? time / count : 0);
}

Rom* Rom::save()
{
    db.save(get_DB_row());
    auto it = std::find_if(list.begin(), list.end(), [&](const Rom& r) { return r.file == file; });
    if (it != list.end())
        return &(*it);
    list.push_back(*this);
    return &list.back();
}

void Rom::remove()
{
    db.remove(file);
    list.erase(
        std::remove_if(list.begin(), list.end(), [this](const Rom& r) { return file == r.file; }),
        list.end());
}

void Rom::fill_opts()
{
    std::string imgBase;
    if (std::regex_search(file, best_pattern))
        imgBase = std::regex_replace(file, best_pattern, R"(/Best/$1/Imgs)");
    else
        imgBase = std::regex_replace(file, img_pattern, R"(/Imgs/$1)");

    image = imgBase + "/" + name + ".png";
    if (!fs::exists(image))
        image = "";

    video = std::regex_replace(file, img_pattern, R"(/Videos/$1)") + "/" + name + ".mp4";
    if (!fs::exists(video))
        video = "";

    manual = std::regex_replace(file, img_pattern, R"(/Manuals/$1)") + "/" + name + ".pdf";
    if (!fs::exists(manual))
        manual = "";

    system = std::regex_replace(file, sys_pattern, R"($1)");
    total_time = utils::stringifyTime(time);
    average_time = utils::stringifyTime(count ? time / count : 0);
    launcher = utils::get_launcher(system, name);
}

Rom::Rom(DB_row row)
    : file(row.file)
    , name(row.name)
    , count(row.count)
    , time(row.time)
    , lastsessiontime(row.lastsessiontime)
    , last(row.last)
    , completed(row.completed)
    , favorite(row.favorite)
{
    fill_opts();
}

Rom::Rom(const std::string& _file, int _time)
    : time(_time)
    , lastsessiontime(_time)
{
    fs::path filepath(_file);

    file = utils::shorten_file_path(_file);
    name = filepath.stem();
    if (time) {
        count = 1;
        last = utils::getCurrentDateTime();
    }
    fill_opts();
}

Rom::Rom(const std::string& _file)
    : Rom(_file, 0)
{
}

Rom::~Rom()
{
    if (pid != -1) {
        utils::resume_process_group(pid);
        utils::kill_process_group(pid);
        gui.message_popup(15, {{"Please wait...", 32, cfg.title_color},
                                  {"We save suspended games.", 18, cfg.title_color}});
    }
}

/**
 * @brief Starts or resumes the game process.
 *
 * @details This function is the primary entry point for launching a game. It first checks if the
 * game is already running and suspended. If so, it resumes the
 * associated process group and restores any necessary hotkey configurations. If the game is not
 * currently running, it verifies that the ROM file exists. If the file is found, it forks a new
 * process and uses `execl` to execute a system-specific launcher script located in
 * `/mnt/SDCARD/Emus/`. The child process's ID is then stored, and a list of running children is
 * exported. If any of these steps fail (e.g., the ROM file doesn't exist or the launcher fails to
 * execute), a popup message is displayed to the user.
 *
 */
void Rom::start()
{
    std::cout << "ActivitiesApp: Launching " << name << std::endl;

    if (pid != -1) {
        utils::resume_process_group(pid);
        std::cout << "Resuming process group: " << name << std::endl;
        // Restore ra_hotkey only if it existed when we suspended this game
        auto it = ra_hotkey_roms.find(file);
        if (it != ra_hotkey_roms.end()) {
            utils::restore_ra_hotkey();
            ra_hotkey_roms.erase(it);
        }
    } else {
        fs::path romPath = fs::path(file);
        if (!fs::exists(romPath)) {
            gui.message_popup(3000, {{"Error", 28, cfg.title_color},
                                        {"The rom file not exist", 18, cfg.selected_color}});
            return;
        }

        pid = fork();
        if (pid == 0) {
            setsid();
            std::string launcher = "/mnt/SDCARD/Emus/" + system + "/default.sh";

            execl(launcher.c_str(), launcher.c_str(), file.c_str(), (char*) NULL);
            std::cerr << "Failed to launch " << name << std::endl;
            exit(1);
        } else {
            childs.insert(file);
            export_childs_list();
        }
    }
}

/**
 * @brief Stops a the currently running game.
 *
 * @details This function terminates the game process. It gets the
 * process group ID of that process and sends a `SIGCONT` signal to the entire group to ensure
 * it's not in a suspended state, as suspended processes cannot be killed. After resuming, it
 * sends a `SIGKILL` signal to terminate the entire process group. Finally, it removes the
 * game from the list of running child processes and updates the exported list.
 *
 */
void Rom::stop()
{
    utils::resume_process_group(pid);
    utils::kill_process_group(pid);
    childs.erase(file);
    pid = -1;
    export_childs_list();
}

/**
 * @brief Suspends the game process.
 *
 * @details This function suspends the child process. It first checks
 * for a specific "hotkey" file (`ra_hotkey_exists()`) which is used to manage in-game menus.
 * If this hotkey file exists, the function adds the `romFile`
 * to a set (`ra_hotkey_roms`) to keep track of games that had the hotkey enabled. This allows for
 * restoration later. It then gets the process ID (PID) and its process group ID (PGID) to suspend
 * the entire process group. Finally, it removes the hotkey file to prevent conflicts while the GUI
 * is active and returns the PID of the suspended process.
 *
 */
void Rom::suspend()
{
    if (utils::ra_hotkey_exists())
        ra_hotkey_roms.insert(file);
    utils::suspend_process_group(pid);
    utils::remove_ra_hotkey();
}

/**
 * @brief Waits for the child process to exit, handling suspension and context switching.
 *
 * @details This function puts the main application into a waiting state while the child
 * game process is running. It continuously polls for the child process's status to
 * detect its termination. It also processes `SDL` events to detect a specific button
 * combination (joystick buttons 6(Select) and 7(Start)) that triggers a context switch.
 * Upon detecting the button combo, it suspends the game process, takes a screenshot
 * to use as a background, and presents a `string_selector` menu to the user.
 * The user can choose to resume the game with B key or switch to another part of the application
 * (e.g., "Running," "Favorites," or "Any"). Depending on the user's choice,
 * the function either resumes the game or returns a specific value to signal a context switch.
 *
 * @return A `std::pair<pid_t, int>`. The `pid_t` is the PID of the process that was
 * being waited on. The integer return code indicates the outcome:
 * - `0`: The game process exited normally or an error occurred.
 * - `1`: The user chose to switch to the "Running" games list.
 * - `2`: The user chose to switch to the "Favorites" games list.
 * - `3`: The user chose to switch to the "Any" games list.
 *
 * @note This function is critical for allowing a seamless transition between a running game
 * and the main GUI while preserving the game's state.
 */
int Rom::wait()
{
    int combo = 0;
    if (pid == -1) {
        std::cerr << "ActivitiesApp: Could not find child process for " << file << std::endl;
        return 0;
    }
    std::cout << "Waiting for " << file << " (PID: " << pid << ")" << std::endl;

    // Pause the GUI interface while the game is running
    int status = 0;

    while (true) {

        // Check if the process still exists
        if (kill(pid, 0) != 0 && errno == ESRCH) {
            std::cout << "ActivitiesApp: Game " << file << " exited (process no longer exists)"
                      << std::endl;
            break;
        }

        // Check if the process has terminated
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            // If the game was stoped by a signal, that mostly because of
            // cmd_to_launch_killer.sh
            // so we not remove it from child to keep it in autostarts
            if (WIFSIGNALED(status)) {
                return 3;
                std::cout << "ActivitiesApp: Game " << file << " exited with status " << status
                          << std::endl;
            }
            break;
        }
        // Pause the GUI interface while the game is running
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                return 0;

            if (e.type == SDL_JOYBUTTONDOWN) {
                switch (e.jbutton.button) {
                case 6: combo |= 1; break;
                case 7: combo |= 2; break;
                default: break;
                }
            } else if (e.type == SDL_JOYBUTTONUP) {
                switch (e.jbutton.button) {
                case 6: combo &= ~1; break;
                case 7: combo &= ~2; break;
                default: break;
                }
            }
            if (combo == 3) {
                combo = 0;
                utils::suspend_process_group(pid);
                gui.save_background_texture(gui.take_screenshot());

                std::vector<std::string> choices;
                choices.push_back("Running");
                choices.push_back("Favorites");
                choices.push_back("Any");
                std::string choice =
                    gui.string_selector("Switch to: ", choices, gui.Width / 3, true);
                if (choice == "") {
                    utils::resume_process_group(pid);
                    gui.delete_background_texture();
                } else {
                    if (utils::ra_hotkey_exists()) {
                        ra_hotkey_roms.insert(file);
                        utils::remove_ra_hotkey();
                    }
                    std::cout << "ActivitiesApp: Game " << file << " suspended" << std::endl;
                    gui.delete_background_texture();
                    if (choice == "Running")
                        return 1;
                    if (choice == "Favorites")
                        return 2;
                    if (choice == "Any")
                        return 3;
                }
            }
        }

        SDL_Delay(100); // Increased delay to reduce CPU load
    }
    pid = -1;
    utils::remove_ra_hotkey();
    ra_hotkey_roms.erase(file);
    childs.erase(file);
    export_childs_list();
    return 0;
}

/**
 * @brief Exports the list of running child processes to a file.
 *
 * @details This function is responsible for persisting the state of currently
 * running game processes. It checks if there are any active child processes
 * stored in the `childs` map. If the map is empty, it deletes the `autostarts.txt`
 * file. Otherwise, it opens the file, clears its contents (`std::ios::trunc`),
 * and writes the path of each running ROM to a new line. This file can be used
 * by other parts of the application or a system service to identify and manage
 * processes that should be automatically restarted or handled upon system boot.
 *
 * @note The function writes only the `romFile` (the key of the `childs` map) to the file.
 */
void Rom::export_childs_list()
{
    if (childs.empty()) {
        std::remove("/mnt/SDCARD/Apps/Activities/data/autostarts.txt");
        return;
    }
    std::ofstream file("/mnt/SDCARD/Apps/Activities/data/autostarts.txt", std::ios::trunc);

    if (!file.fail()) {
        for (const std::string& romFile : childs)
            file << romFile << std::endl;
        file.flush();
        file.close();
    }
}
