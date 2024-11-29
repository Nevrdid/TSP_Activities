#include "Timer.h"

Timer::Timer(const std::string& pid)
    : running(true)
{
    std::string stat_path = "/proc/" + pid + "/stat";
    fd = open(stat_path.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open" << stat_path << std::endl;
        return;
    }
    if (instance) {
        std::cerr << "Timer instance already exists" << std::endl;
        return;
    }
    instance = this;
}

Timer::~Timer()
{
    if (fd != -1)
        close(fd);
    instance = nullptr;
}

void Timer::timer_handler(int signum)
{
    (void) signum;
    if (!instance || !instance->running)
        return;

    if (lseek(instance->fd, 0, SEEK_SET) == -1) {
        instance->running = false;
        return;
    }

    char    buffer[128];
    ssize_t bytes_read = read(instance->fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        instance->running = false;
        return;
    }

    int space_count = 0;
    for (ssize_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == ' ')
            space_count++;
        if (space_count == 2 && ++i < bytes_read) {
            char state = buffer[i];
            if (state == 'Z')
                instance->running = false;
            break;
        }
    }

    instance->tick_counter++;
    if (instance->tick_counter == 4) {
        instance->elapsed_seconds++;
        instance->tick_counter = 0;
    }
}

unsigned long Timer::run()
{
    struct sigaction sa;
    sa.sa_handler = Timer::timer_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, nullptr);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 250000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 250000;

    if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
        std::cerr << "Failed to set timer" << std::endl;
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
