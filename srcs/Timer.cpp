#include "Timer.h"

Timer* Timer::instance = nullptr;

Timer::Timer(const string& pid)
    : elapsed_seconds(0)
    , running(true)
{
    std::string stat_path = "/proc/" + pid + "/stat";
    instance = this;
    fd = open(stat_path.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open " << stat_path << std::endl;
    }
}

Timer::~Timer()
{
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    instance = nullptr;
}

/* Get the state of the target process by PID
 * 0: Stopped/Suspended
 * 1: Running
 * -1: Not found/Terminated
 */

void Timer::timer_handler(int signum)
{
    if (instance == nullptr) return;

    if (lseek(instance->fd, 0, SEEK_SET) == -1) {
        close(instance->fd);
        instance->running = false;
        return;
    }

    char    buffer[64];
    ssize_t bytes_read = read(instance->fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(instance->fd);
        instance->running = false;
        return;
    }

    int space_count = 0;
    for (ssize_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == ' ') space_count++;
        if (space_count == 2 && ++i < bytes_read) {
            char state = buffer[i];
            if (state == 'Z') instance->running = false;
            else if (state != 'T') instance->elapsed_seconds++;
            break;
        }
    }
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
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, nullptr);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;

    if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
        throw std::runtime_error("Failed to set timer");
    }

    while (running) {
        pause();
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, nullptr);

    return elapsed_seconds;
}
