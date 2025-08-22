#include <GameRunner.h>

GameRunner::GameRunner(GUI& gui)
    : gui(gui)
{
}

GameRunner::~GameRunner()
{
    if (childs.empty()) {
        gui.message_popup("Leaving...", 32, "Good bye", 32, 1000);
        return;
    }
    int status;
    gui.message_popup("Please wait...", 32, "We save suspended games.", 18, 15);
    for (const std::pair<std::string, pid_t> child : childs) {
        pid_t gpid = utils::get_pgid_of_process(child.second);
        utils::resume_process_group(gpid);
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

void GameRunner::stop(const std::string& romFile)
{
    pid_t pid = get_child_pid(romFile);
    pid_t gpid = utils::get_pgid_of_process(pid);
    utils::resume_process_group(gpid);
    utils::kill_process_group(pid);
    childs.erase(romFile);
    export_childs_list();
}

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
            childs[romFile] = pid;
            export_childs_list();
        }
    }
}

void GameRunner::add_child(const std::string& romFile, int pid)
{
    childs[romFile] = pid;
}

pid_t GameRunner::get_child_pid(const std::string& romFile)
{
    auto it = childs.find(romFile);
    if (it == childs.end())
        return -1;
    return it->second;
}

pid_t GameRunner::wait(const std::string& romFile)
{
    pid_t pid = get_child_pid(romFile);
    if (pid == -1) {
        std::cerr << "ActivitiesApp: Could not find child process for " << romFile << std::endl;
        return -1;
    }
    std::cout << "GameRunner: Waiting for " << romFile << " (PID: " << pid << ")" << std::endl;

    // Pause the GUI interface while the game is running
    int combo = 0;
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
            // If the game was stoped by a signal, that mostly because of cmd_to_launch_killer.sh
            //    // so we not remove it from child to keep it in autostarts
            if (WIFSIGNALED(status))
                return pid;
            std::cout << "ActivitiesApp: Game " << romFile << " exited with status " << status
                      << std::endl;
            break;
        }
        // Pause the GUI interface while the game is running
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                return pid;

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
                    ra_hotkey_roms.insert(romFile);
                utils::suspend_process_group(utils::get_pgid_of_process(pid));
                std::cout << "ActivitiesApp: Game " << romFile << " suspended" << std::endl;
                // Returning to GUI (suspended): mark and remove ra_hotkey now
                utils::remove_ra_hotkey();
                return pid;
            }
        }

        SDL_Delay(100); // Increased delay to reduce CPU load
    }
    utils::remove_ra_hotkey();
    ra_hotkey_roms.erase(romFile);
    childs.erase(romFile);
    export_childs_list();
    return -1;
}

pid_t GameRunner::suspend(const std::string& romFile)
{
    if (utils::ra_hotkey_exists())
        ra_hotkey_roms.insert(romFile);
    pid_t pid = get_child_pid(romFile);
    utils::suspend_process_group(utils::get_pgid_of_process(pid));
    utils::remove_ra_hotkey();
    return pid;
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
