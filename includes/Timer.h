#pragma once

#include <fcntl.h>
#include <string>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

class Timer
{
  private:
    Timer();
    Timer(const std::string& pid);
    Timer(const Timer& copy);
    Timer& operator=(const Timer& copy);

    // Timer(const Timer&) = delete;
    // Timer& operator=(const Timer&) = delete;

    int                    fd = -1;
    volatile bool          running = false;
    volatile bool          suspended = false;
    volatile unsigned long elapsed_seconds = 0;
    volatile unsigned int  tick_counter = 0;

  public:
    ~Timer();

    static Timer& getInstance(const std::string& pid = "")
    {
        static Timer instance(pid);
        return instance;
    }

    static void daemonize(const std::string& rom_file, const std::string& program_pid);
    static void timer_handler(int signum);

    long run();
};
