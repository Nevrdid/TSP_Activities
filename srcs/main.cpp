#include "Activities.h"
#include "DB.h"
#include "Timer.h"
#include <wait.h>
#include <cstring>

#define HELP_MESSAGE                                                                               \
    "Usage:\n\
  activities time <activity_name> <pid>\n\
  activities time <activity_name> -flag <file_path>\n\
  activities gui [rom_file]"

Timer* Timer::instance = nullptr;

// Daemonize the timer to avoid it beeing killed before it saved time.
void timer_daemonize(const std::string& rom_file, const std::string& program_pid)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Failed to create pipe" << std::endl;
        exit(1);
    }

    // Fork to create detached process
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        exit(1);
    }

    if (pid > 0) {
        close(pipe_fd[1]);

        // Original process just wait daemon to terminate
        char buffer[1];
        while (read(pipe_fd[0], buffer, 1) > 0) {
        }
        close(pipe_fd[0]);

        waitpid(pid, nullptr, 0);
        return;
    }

    close(pipe_fd[0]);

    // Leaving original session
    if (setsid() == -1) {
        std::cerr << "Failed to start new session" << std::endl;
        exit(1);
    }

    // Create the daemon
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

    Timer        timer(program_pid);
    unsigned int duration = timer.run();

    DB db;
    db.save(rom_file, duration);

    // Notify original process save is finish
    close(pipe_fd[1]);
}

// Daemonize the file watcher
void file_watcher_daemonize(const std::string& activity_name, const std::string& file_path)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Failed to create pipe" << std::endl;
        exit(1);
    }

    // Fork to create detached process
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        exit(1);
    }

    if (pid > 0) {
        close(pipe_fd[1]);

        // Original process just wait daemon to terminate
        char buffer[1];
        while (read(pipe_fd[0], buffer, 1) > 0) {
        }
        close(pipe_fd[0]);

        waitpid(pid, nullptr, 0);
        return;
    }

    close(pipe_fd[0]);

    // Leaving original session
    if (setsid() == -1) {
        std::cerr << "Failed to start new session" << std::endl;
        exit(1);
    }

    // Create the daemon
    pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork again" << std::endl;
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    int logfile =
        open("/mnt/SDCARD/Apps/Activities/log/file_watcher.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    dup2(logfile, STDOUT_FILENO);
    dup2(logfile, STDERR_FILENO);
    close(logfile);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull != -1) {
        dup2(devnull, STDIN_FILENO);
        close(devnull);
    }

    Timer        timer(file_path, true);
    unsigned int duration = timer.run();

    DB db;
    db.save(activity_name, duration);

    // Notify original process save is finish
    close(pipe_fd[1]);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << HELP_MESSAGE << std::endl;
        return 1;
    }

    if (std::strcmp(argv[1], "time") == 0) {
        if (argc == 4) {
            timer_daemonize(argv[2], argv[3]);
        } else if (argc == 5 && std::strcmp(argv[3], "-flag") == 0) {
            file_watcher_daemonize(argv[2], argv[4]);
        } else {
            std::cout << HELP_MESSAGE << std::endl;
        }
    } else if (std::strcmp(argv[1], "gui") == 0) {
        Activities app(argc > 2 ? argv[2] : "");
        app.run();
    } else {
        std::cout << HELP_MESSAGE << std::endl;
    }

    return 0;
}
