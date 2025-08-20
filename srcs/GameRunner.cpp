#include <GameRunner.h>

GameRunner::GameRunner(GUI& gui)
    : gui(gui)
{
}

GameRunner::~GameRunner()
{}

void GameRunner::stop_all()
{
    if (childs.empty()) {
        gui.message_popup("Leaving...", 32, "Good bye", 32, 1000);
        return;
    }
    int status;
    gui.message_popup("Please wait...", 32, "We save suspended games.", 18, 15);
    for (const std::pair<std::string, pid_t> child : childs) {
        utils::resume_process_group(child.second);
        utils::kill_process_group(child.second);
        gui.message_popup("Please wait...", 32, "We save suspended games.", 18, 15);
    }
    while (waitpid(-1, &status, WNOHANG) > -1) {
        gui.message_popup("Please wait...", 32, "We save suspended games.", 18, 15);
    }

    // Fix some child keep erasing fb.
    for (int i = 0; i < 30; i++)
        gui.message_popup("Good bye", 32, "Your games are saved.", 18, 15);
}

void GameRunner::start(
    const std::string& romName, const std::string& system, const std::string& romFile)
{
    std::cout << "ActivitiesApp: Launching " << romName << std::endl;

    if (childs.find(romName) != childs.end()) {
        pid_t pid = childs[romName];
        pid_t gpid = utils::get_pgid_of_process(pid);
        utils::resume_process_group(gpid);
        std::cout << "Resuming process group: " << romName << std::endl;
        // Restore ra_hotkey only if it existed when we suspended this game
        auto it = ra_hotkey_roms.find(romName);
        if (it != ra_hotkey_roms.end()) {
            utils::restore_ra_hotkey();
            ra_hotkey_roms.erase(it);
        }
    } else {
        fs::path romPath = fs::path(romFile);
        if (!fs::exists(romPath)) {
            gui.message_popup("Error", 28, "The rom file not exist", 18, 3000);
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
            childs[romName] = pid;
        }
    }
}

pid_t GameRunner::get_child_pid(const std::string& romName)
{
    auto it = childs.find(romName);
    if (it == childs.end())
        return -1;
    return it->second;
}

pid_t GameRunner::wait(const std::string& romName)
{
    pid_t pid = get_child_pid(romName);
    if (pid == -1) {
        std::cerr << "ActivitiesApp: Could not find child process for " << romName << std::endl;
        return -1;
    }
    std::cout << "GameRunner: Waiting for " << romName << " (PID: " << pid << ")" << std::endl;

    // Pause the GUI interface while the game is running
    int combo = 0;
    int status = 0;

    while (true) {
        // Check if the process still exists
        if (kill(pid, 0) != 0 && errno == ESRCH) {
            std::cout << "ActivitiesApp: Game " << romName << " exited (process no longer exists)"
                      << std::endl;
            break;
        }

        // Check if the process has terminated
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            std::cout << "ActivitiesApp: Game " << romName << " exited with status " << status
                      << std::endl;
            break;
        }
        // Pause the GUI interface while the game is running
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
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
                // Before suspending, remember if ra_hotkey existed to restore later
                if (utils::ra_hotkey_exists())
                    ra_hotkey_roms.insert(romName);
                utils::suspend_process_group(utils::get_pgid_of_process(pid));
                std::cout << "ActivitiesApp: Game " << romName << " suspended" << std::endl;
                // Returning to GUI (suspended): mark and remove ra_hotkey now
                utils::remove_ra_hotkey();
                return pid;
            }
        }

        SDL_Delay(100); // Increased delay to reduce CPU load
    }
    utils::remove_ra_hotkey();
    ra_hotkey_roms.erase(romName);
    childs.erase(romName);
    return -1;
}

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
