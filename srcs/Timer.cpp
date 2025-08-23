#include "Timer.h"

#include <chrono>
#include <sys/select.h>

Timer::Timer(const std::string& pid)
    : running(true)
    , is_file_mode(false)
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

Timer::Timer(const std::string& file_path, bool file_mode)
    : running(true)
    , is_file_mode(file_mode)
    , file_to_watch(file_path)
{
    if (!file_mode) {
        std::cerr << "Invalid constructor call" << std::endl;
        return;
    }

    // Extract directory and filename
    fs::path path(file_path);
    watch_dir = path.parent_path().string();
    watch_filename = path.filename().string();

    // Initialize inotify
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return;
    }

    // Watch the directory for file creation/deletion
    watch_fd = inotify_add_watch(
        inotify_fd, watch_dir.c_str(), IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
    if (watch_fd == -1) {
        std::cerr << "Failed to add watch on directory: " << watch_dir << std::endl;
        close(inotify_fd);
        inotify_fd = -1;
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
    if (watch_fd != -1 && inotify_fd != -1)
        inotify_rm_watch(inotify_fd, watch_fd);
    if (inotify_fd != -1)
        close(inotify_fd);
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
            switch (buffer[i]) {
            case 'Z': instance->running = false; return;
            case 'T':
                // Keep sum session between sleep until 30 seconds min are reached.
                if (instance->elapsed_seconds > 30)
                    instance->suspended = true;
                return;
            default: break;
            }
            break;
        }
    }
    instance->tick_counter++;
    if (instance->tick_counter == 4) {
        instance->elapsed_seconds++;
        instance->tick_counter = 0;
    }
}

bool Timer::file_exists() const
{
    return fs::exists(file_to_watch);
}

unsigned long Timer::run_file_watch()
{
    if (inotify_fd == -1) {
        std::cerr << "inotify not initialized" << std::endl;
        return 0;
    }

    bool file_present = file_exists();
    auto last_check = std::chrono::steady_clock::now();

    // If file exists at start, begin counting
    if (file_present) {
        elapsed_seconds = 0;
        std::cout << "File exists at start, tracking time..." << std::endl;
    }

    while (running) {
        char           buffer[4096];
        fd_set         readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(inotify_fd, &readfds);

        // Check every second using select timeout
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(inotify_fd + 1, &readfds, nullptr, nullptr, &timeout);

        if (ready == -1) {
            std::cerr << "select() error" << std::endl;
            break;
        }

        // Check if file events occurred
        if (ready > 0 && FD_ISSET(inotify_fd, &readfds)) {
            ssize_t len = read(inotify_fd, buffer, sizeof(buffer));
            if (len > 0) {
                struct inotify_event* event;
                for (char* ptr = buffer; ptr < buffer + len;
                     ptr += sizeof(struct inotify_event) + event->len) {
                    event = reinterpret_cast<struct inotify_event*>(ptr);

                    // Check if the event concerns our target file
                    if (event->len > 0 && std::string(event->name) == watch_filename) {
                        if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
                            if (!file_present) {
                                file_present = true;
                                std::cout << "File created/moved in: " << file_to_watch
                                          << std::endl;
                            }
                        } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                            if (file_present) {
                                file_present = false;
                                std::cout << "File deleted/moved out: " << file_to_watch
                                          << std::endl;
                                // Stop tracking when file disappears
                                running = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Update elapsed time every second if file is present
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() >= 1) {
            if (file_present) {
                elapsed_seconds++;
            }
            last_check = now;
        }
    }

    std::cout << "File watch ended. Total time: " << elapsed_seconds << " seconds" << std::endl;
    return elapsed_seconds;
}

long Timer::run()
{
    if (is_file_mode) {
        return run_file_watch();
    }

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
