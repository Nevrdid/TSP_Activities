#pragma once

#if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#    error "No filesystem support"
#endif

#include "utils.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/inotify.h>
#include <sys/time.h>
#include <unistd.h>

class Timer
{
  private:
    Timer();
    Timer(const std::string& pid);
    Timer(const std::string& file_path, bool file_mode);
    Timer(const Timer& copy);
    Timer& operator=(const Timer& copy);

    // Timer(const Timer&) = delete;
    // Timer& operator=(const Timer&) = delete;

    int                    fd = -1;
    int                    inotify_fd = -1;
    int                    watch_fd = -1;
    volatile bool          running = false;
    volatile bool          suspended = false;
    bool                   is_file_mode = false;
    std::string            file_to_watch;
    std::string            watch_dir;
    std::string            watch_filename;
    volatile unsigned long elapsed_seconds = 0;
    volatile unsigned int  tick_counter = 0;

  public:
    ~Timer();

    static Timer& getInstance(const std::string& pid = "") {
      static Timer instance(pid);
      return instance;
    }
    static Timer& getInstance(const std::string& file_path, bool file_mode) {
      static Timer instance(file_path, file_mode);
      return instance;
    }

    static void   timer_handler(int signum);
    long          run();
    unsigned long run_file_watch();
    bool          file_exists() const;
};
