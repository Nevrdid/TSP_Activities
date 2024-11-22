#include "Timer.h"

Timer::Timer(const string& rom_file, const string& pid)
    : target_rom(rom_file)
    , target_pid(pid)
    , elapsed_seconds(0)
{
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
    std::string ps_command = "ps -p " + target_pid + " -o state= | tr -d ' '";
    FILE*       ps = popen(ps_command.c_str(), "r");

    if (ps) {
        char state[3];
        if (fgets(state, sizeof(state), ps)) {
            pclose(ps);
            if (state[0] == 'T') {
                return 0;
            } else if (state[0] == 'Z' || state[0] == 'X') {
                return -1;
            }
            return 1;
        }
        pclose(ps);
    }
    return -1;
}
