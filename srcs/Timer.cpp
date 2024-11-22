#include "Timer.h"

Timer::Timer(const string& rom_file, const string& pid)
    : target_rom(rom_file)
    , target_pid(pid)
    , elapsed_seconds(0)
{
    stat_path = "/proc/" + target_pid + "/stat";
}

Timer::~Timer()
{
}

/* Start the timer
 * Check the state of the target process every 15 seconds
 * If the target process is running, increment the elapsed time
 * If the target process is stopped, do nothing
 * If the target process is terminated, stop the timer
 */
unsigned long Timer::run()
{
    while (true) {
        int state = get_target_state();
        sleep(5);
        if (state == 1) elapsed_seconds += 5;
        else if (state == -1) break;
    }
    return elapsed_seconds;
}

/* Get the state of the target process by PID
 * 0: Stopped/Suspended
 * 1: Running
 * -1: Not found/Terminated
 */
int Timer::get_target_state()
{
    std::ifstream stat_file(stat_path);
    if (!stat_file.is_open()) {
        return -1;
    }

    std::string line;
    if (std::getline(stat_file, line)) {
        std::istringstream iss(line);
        std::string        dummy;
        char               state;

        iss >> dummy >> dummy >> state;

        if (state == 'T') {
            return 0;
        } else if (state == 'Z') {
            return -1;
        } else {
            return 1;
        }
    }

    return -1;
}
