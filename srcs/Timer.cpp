#include "Timer.h"

#include "Rom.h"

#include <iostream>
#include <ostream>
#include <sys/select.h>

Timer::Timer(const std::string& pid)
    : running(true)
{
    std::string stat_path = "/proc/" + pid + "/stat";
    fd = open(stat_path.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open" << stat_path << std::endl;
        return;
    }
}

Timer::~Timer()
{
    if (fd != -1)
        close(fd);
}

// Daemonize the timer to avoid it being killed before it saves time.
void Timer::daemonize(const std::string& rom_file, const std::string& program_pid)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Failed to create pipe" << std::endl;
        exit(1);
    }

    // Fork to create a detached process
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        exit(1);
    }

    if (pid > 0) {
        close(pipe_fd[1]);

        // Original process just waits for the daemon to terminate
        char buffer[1];
        while (read(pipe_fd[0], buffer, 1) > 0) {
        }
        close(pipe_fd[0]);

        waitpid(pid, nullptr, 0);
        return;
    }

    close(pipe_fd[0]);

    // Leave the original session
    if (setsid() == -1) {
        std::cerr << "Failed to start new session" << std::endl;
        exit(1);
    }

    // Create the daemon process
    pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork again" << std::endl;
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    int logfile =
        open("/mnt/SDCARD/Apps/Activities/log/timer.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    dup2(logfile, STDOUT_FILENO);
    dup2(logfile, STDERR_FILENO);
    close(logfile);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull != -1) {
        dup2(devnull, STDIN_FILENO);
        close(devnull);
    }

    long duration = -1;
    // a negative duration mean session end with game beeing suspended.
    Timer& timer = Timer::getInstance(program_pid);
    while (duration < 0) {
        duration = timer.run();
        if (std::abs(duration) >= 30) {
            Rom rom(rom_file, std::abs(duration));
            rom.save();
        }
    }

    // Notify the original process that saving is finished
    close(pipe_fd[1]);
}

void Timer::timer_handler(int signum)
{
    Timer& instance = Timer::getInstance();
    (void) signum;
    if (!instance.running)
        return;

    if (lseek(instance.fd, 0, SEEK_SET) == -1) {
        instance.running = false;
        return;
    }
    char    buffer[128];
    ssize_t bytes_read = read(instance.fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        instance.running = false;
        return;
    }
    int space_count = 0;
    for (ssize_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == ' ')
            space_count++;
        if (space_count == 2 && ++i < bytes_read) {
            switch (buffer[i]) {
            case 'Z': instance.running = false; return;
            case 'T':
                // Keep sum session between sleep until 30 seconds min are reached.
                if (instance.elapsed_seconds > 30)
                    instance.suspended = true;
                return;
            default: break;
            }
            break;
        }
    }
    instance.tick_counter++;
    if (instance.tick_counter == 4) {
        instance.elapsed_seconds++;
        instance.tick_counter = 0;
    }
}

long Timer::run()
{
    elapsed_seconds = 0;
    suspended = false;
    running = true;

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

    while (running && !suspended) {
        pause();
    }

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, nullptr);

    return suspended ? -elapsed_seconds : elapsed_seconds;
}
