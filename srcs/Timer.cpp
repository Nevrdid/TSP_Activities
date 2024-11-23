#include "Timer.h"

Timer* Timer::instance = nullptr;

Timer::Timer(const string& rom_file, const string& pid)
    : target_rom(rom_file)
    , target_pid(pid)
    , elapsed_seconds(0)
    , running(true)
{
    stat_path = "/proc/" + target_pid + "/stat";
    instance = this;
}

Timer::~Timer()
{
    instance = nullptr;
}

void Timer::timer_handler(int signum)
{
    if (instance == nullptr) return;

    int state = instance->get_target_state();

    if (state == 1) {
        instance->elapsed_seconds++;
    } else if (state == -1) {
        std::cout << "Process terminated. Stopping timer.\n";
        instance->running = false;
    }
    // state == 0, wait next signal
}
/* Start the timer
 * Check the state of the target process every 15 seconds
 * If the target process is running, increment the elapsed time
 * If the target process is stopped, do nothing
 * If the target process is terminated, stop the timer
 */
unsigned long Timer::run()
{
    struct sigaction sa;
    sa.sa_handler = Timer::timer_handler;
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    sigaction(SIGALRM, &sa, nullptr);

    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
        throw std::runtime_error("Failed to set timer");
    }

    std::cout << "Timer started. Monitoring process " << target_pid << "...\n";

    while (running) {
        pause(); // Wait for signals
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, nullptr);

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
