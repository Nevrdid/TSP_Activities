#include "Activities.h"
#include "DB.h"
#include "Timer.h"
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "No filesystem support"
#endif
static const char timer_help[] = {
  "activities Timer usage:\n"
  "\t activities time [option...]* <romFile> <processPID>\n"
};

static const char global_help[] = {
  "activities usage:\n"
  "\t activities [command] [options] ...\n"
  "Commands:\n"
  "\t- gui: Display the gui\n"
  "\t- time: Time a game and add it to the DB.\n"
  "\n*use `activities [command] -h for details\n"
};

Timer* Timer::instance = nullptr;

// Daemonize the timer to avoid it being killed before it saves time.
void timer_daemonize(const std::string& rom_file, const std::string& program_pid)
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

    Timer        timer(program_pid);
    unsigned int duration = timer.run();

    DB db;
    db.save(rom_file, duration);

    // Notify the original process that saving is finished
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

    int devnull = open("/dev/null", O_RDWR);
    if (devnull != -1) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
    }

    Timer        timer(file_path, true);
    unsigned int duration = timer.run();

    DB db;
    db.save(activity_name, duration);

    // Notify the original process that saving is finished
    close(pipe_fd[1]);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << global_help << std::endl;
        return 1;
    }

    if (std::strcmp(argv[1], "time") == 0) {
        if (argc == 4) {
            timer_daemonize(argv[2], argv[3]);
        } else if (argc == 5 && std::strcmp(argv[3], "-flag") == 0) {
            // Extract the system from the rom path to build the launcher path
            std::string romPath = argv[2];
            fs::path p(romPath);
            std::string system = p.parent_path().parent_path().filename().string();

            // Create the temporary script for monitoring
            std::ofstream script("/tmp/cmd_to_run.sh");
            script << "#!/bin/sh\n";
            script << "exec /mnt/SDCARD/Emus/" << system << "/default.sh '" << romPath << "'\n";
            script.close();
            chmod("/tmp/cmd_to_run.sh", 0755);

            file_watcher_daemonize(argv[2], argv[4]);
        } else {
            std::cout << timer_help << std::endl;
        }
    } else if (std::strcmp(argv[1], "gui") == 0) {
        Activities app;
        // app runner will handle himself if argv[2] is a romfile or a flag.
        app.run(argc,argv);
    } else {
        std::cout << global_help << std::endl;
    }

    return 0;
}
