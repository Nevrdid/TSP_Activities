#include <GameRunner.h>

GameRunner::GameRunner()
    : cfg(Config::getInstance())
    , gui(GUI::getInstance())
{
}

GameRunner::~GameRunner()
{
    gui.save_background_texture();
    if (childs.empty()) {
        gui.message_popup(
            2000, {{"Leaving...", 32, cfg.title_color}, {"Good bye", 32, cfg.selected_color}});
        gui.delete_background_texture();
        return;
    }
    int status;
    gui.message_popup(15, {{"Please wait...", 32, cfg.title_color},
                              {"We save suspended games.", 18, cfg.title_color}});

    for (const std::pair<std::string, pid_t> child : childs) {
        pid_t gpid = utils::get_pgid_of_process(child.second);
        utils::resume_process_group(gpid);
        utils::kill_process_group(child.second);
        gui.message_popup(15, {{"Please wait...", 32, cfg.title_color},
                                  {"We save suspended games.", 18, cfg.title_color}});
    }
    while (waitpid(-1, &status, WNOHANG) > -1) {
        gui.message_popup(15, {{"Please wait...", 32, cfg.title_color},
                                  {"We save suspended games.", 18, cfg.title_color}});
    }

    // Fix some child keep erasing fb.
    for (int i = 0; i < 30; i++)
        gui.message_popup(15,
            {{"Good bye", 32, cfg.title_color}, {"Your games are saved.", 18, cfg.title_color}});

    // gui.delete_background_texture();
}

/**
 * @brief Starts or resumes a game/emulator process.
 *
 * @details This function is the primary entry point for launching a game. It first checks if the
 * game, identified by its `romFile`, is already running and suspended. If so, it resumes the
 * associated process group and restores any necessary hotkey configurations. If the game is not
 * currently running, it verifies that the ROM file exists. If the file is found, it forks a new
 * process and uses `execl` to execute a system-specific launcher script located in
 * `/mnt/SDCARD/Emus/`. The child process's ID is then stored, and a list of running children is
 * exported. If any of these steps fail (e.g., the ROM file doesn't exist or the launcher fails to
 * execute), a popup message is displayed to the user.
 *
 * @param romName A constant reference to a string containing the display name of the ROM.
 * @param system A constant reference to a string containing the name of the system (e.g., "NES",
 * "SNES").
 * @param romFile A constant reference to a string containing the full path to the ROM file.
 */
void GameRunner::start(
    const std::string& romName, const std::string& system, const std::string& romFile)
{
    std::cout << "ActivitiesApp: Launching " << romName << std::endl;

    if (childs.find(romFile) != childs.end()) {
        pid_t pid = childs[romFile];
        pid_t gpid = utils::get_pgid_of_process(pid);
        utils::resume_process_group(gpid);
        std::cout << "Resuming process group: " << romName << std::endl;
        // Restore ra_hotkey only if it existed when we suspended this game
        auto it = ra_hotkey_roms.find(romFile);
        if (it != ra_hotkey_roms.end()) {
            utils::restore_ra_hotkey();
            ra_hotkey_roms.erase(it);
        }
    } else {
        fs::path romPath = fs::path(romFile);
        if (!fs::exists(romPath)) {
            gui.message_popup(3000, {{"Error", 28, cfg.title_color},
                                        {"The rom file not exist", 18, cfg.selected_color}});
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            std::string launcher = "/mnt/SDCARD/Emus/" + system + "/default.sh";

            execl(launcher.c_str(), launcher.c_str(), romFile.c_str(), (char*) NULL);
            std::cerr << "Failed to launch " << romName << std::endl;
            exit(1);
        } else {
            childs[romFile] = pid;
            export_childs_list();
        }
    }
}

/**
 * @brief Stops a currently running game/emulator process.
 *
 * @details This function terminates a game process that was previously started by `GameRunner`.
 * It first retrieves the process ID associated with the specified ROM file. It then gets the
 * process group ID of that process and sends a `SIGCONT` signal to the entire group to ensure
 * it's not in a suspended state, as suspended processes cannot be killed. After resuming, it
 * sends a `SIGKILL` signal to terminate the entire process group. Finally, it removes the
 * game from the list of running child processes and updates the exported list.
 *
 * @param romFile A constant reference to a string containing the full path to the ROM file,
 * which is used to identify the process to be stopped.
 */
void GameRunner::stop(const std::string& romFile)
{
    pid_t pid = get_child_pid(romFile);
    pid_t gpid = utils::get_pgid_of_process(pid);
    utils::resume_process_group(gpid);
    utils::kill_process_group(pid);
    childs.erase(romFile);
    export_childs_list();
}

/**
 * @brief Suspends a running game/emulator process.
 *
 * @details This function suspends a child process identified by its ROM file path. It first checks
 * for a specific "hotkey" file (`ra_hotkey_exists()`) which is often used by emulators (like
 * RetroArch) to manage in-game menus. If this hotkey file exists, the function adds the `romFile`
 * to a set (`ra_hotkey_roms`) to keep track of games that had the hotkey enabled. This allows for
 * restoration later. It then gets the process ID (PID) and its process group ID (PGID) to suspend
 * the entire process group. Finally, it removes the hotkey file to prevent conflicts while the GUI
 * is active and returns the PID of the suspended process.
 *
 * @param romFile A constant reference to a string containing the full path to the ROM file.
 * @return The process ID (`pid_t`) of the suspended child process.
 */
pid_t GameRunner::suspend(const std::string& romFile)
{
    if (utils::ra_hotkey_exists())
        ra_hotkey_roms.insert(romFile);
    pid_t pid = get_child_pid(romFile);
    utils::suspend_process_group(utils::get_pgid_of_process(pid));
    utils::remove_ra_hotkey();
    return pid;
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
void GameRunner::export_childs_list()
{
    if (childs.empty()) {
        remove("/mnt/SDCARD/Apps/Activities/data/autostarts.txt");
        return;
    }
    std::ofstream file("/mnt/SDCARD/Apps/Activities/data/autostarts.txt", std::ios::trunc);

    if (!file.fail()) {
        for (const auto& pair : childs)
            file << pair.first << std::endl;
        file.flush();
        file.close();
    }
}

/**
 * @brief Adds a child process to the tracking list.
 *
 * @details This function associates a given ROM file path with its corresponding
 * process ID (PID) and stores this relationship in the `childs` map. This allows
 * the `GameRunner` to keep track of currently active game processes for management
 * operations like stopping or resuming them later.
 *
 * @param romFile A constant reference to a string representing the full path to
 * the ROM file. This serves as the key for tracking the process.
 * @param pid An integer representing the process ID of the launched emulator or game.
 */
void GameRunner::add_child(const std::string& romFile, int pid)
{
    childs[romFile] = pid;
}

/**
 * @brief Retrieves the process ID (PID) of a running child process.
 *
 * @details This function searches the internal map of active child processes (`childs`)
 * to find the PID associated with a given ROM file path. It's a utility function used
 * by other methods in the class to perform operations like stopping or resuming a game.
 *
 * @param romFile A constant reference to a string representing the full path to the
 * ROM file whose process ID is needed.
 * @return The process ID (`pid_t`) of the child process if found; otherwise, returns
 * `-1` to indicate that no process was found for the given `romFile`.
 */
pid_t GameRunner::get_child_pid(const std::string& romFile)
{
    auto it = childs.find(romFile);
    if (it == childs.end())
        return -1;
    return it->second;
}

/**
 * @brief Waits for a child process to exit, handling suspension and context switching.
 *
 * @details This function puts the main application into a waiting state while a child
 * game process is running. It continuously polls for the child process's status to
 * detect its termination. It also processes `SDL` events to detect a specific button
 * combination (joystick buttons 6(Select) and 7(Start)) that triggers a context switch.
 * Upon detecting the button combo, it suspends the game process, takes a screenshot
 * to use as a background, and presents a `string_selector` menu to the user.
 * The user can choose to resume the game with B key or switch to another part of the application
 * (e.g., "Running," "Favorites," or "Any"). Depending on the user's choice,
 * the function either resumes the game or returns a specific value to signal a context switch.
 *
 * @param romFile A constant reference to a string containing the full path of the ROM
 * file, used to identify the child process to wait for.
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
std::pair<pid_t, int> GameRunner::wait(const std::string& romFile)
{
    int   combo = 0;
    pid_t pid = get_child_pid(romFile);
    if (pid == -1) {
        std::cerr << "ActivitiesApp: Could not find child process for " << romFile << std::endl;
        return {pid, 0};
    }
    std::cout << "GameRunner: Waiting for " << romFile << " (PID: " << pid << ")" << std::endl;

    // Pause the GUI interface while the game is running
    int status = 0;

    while (true) {

        // Check if the process still exists
        if (kill(pid, 0) != 0 && errno == ESRCH) {
            std::cout << "ActivitiesApp: Game " << romFile << " exited (process no longer exists)"
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
                return {pid, 3};
                std::cout << "ActivitiesApp: Game " << romFile << " exited with status " << status
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
                return {pid, 0};

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
                utils::suspend_process_group(utils::get_pgid_of_process(pid));
                gui.save_background_texture(gui.take_screenshot());

                std::vector<std::string> choices;
                choices.push_back("Running");
                choices.push_back("Favorites");
                choices.push_back("Any");
                std::string choice =
                    gui.string_selector("Switch to: ", choices, gui.Width / 3, true);
                if (choice == "") {
                    utils::resume_process_group(utils::get_pgid_of_process(pid));
                    gui.delete_background_texture();
                } else {
                    if (utils::ra_hotkey_exists()) {
                        ra_hotkey_roms.insert(romFile);
                        utils::remove_ra_hotkey();
                    }
                    std::cout << "ActivitiesApp: Game " << romFile << " suspended" << std::endl;
                    gui.delete_background_texture();
                    if (choice == "Running")
                        return {pid, 1};
                    if (choice == "Favorites")
                        return {pid, 2};
                    if (choice == "Any")
                        return {pid, 3};
                }
            }
        }

        SDL_Delay(100); // Increased delay to reduce CPU load
    }
    utils::remove_ra_hotkey();
    ra_hotkey_roms.erase(romFile);
    childs.erase(romFile);
    export_childs_list();
    return {-1, 0};
}

/**
 * @brief Starts an external application by executing a shell command.
 *
 * @details This function is designed to launch an external process, such as a game or
 * tool, that is not managed as a child process by the `GameRunner`. It temporarily
 * disables SDL joystick button events to prevent unwanted input from being processed
 * while the external command is running. The command is executed using `system()`,
 * which is a blocking call, meaning the function will not return until the external
 * process has completed. After the command finishes, it re-enables the joystick events
 * and waits for an event to clear the event queue, ensuring the GUI is ready to
 * receive input again.
 *
 * @param command A constant reference to a string containing the shell command to
 * be executed.
 */
void GameRunner::start_external(const std::string& command)
{
    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_DISABLE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_DISABLE);

    std::cout << "ActivitiesApp: Launching " << command << std::endl;

    system(command.c_str());

    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);

    while (SDL_WaitEvent(NULL) < 0) {
    }

    SDL_FlushEvents(SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP);
}
