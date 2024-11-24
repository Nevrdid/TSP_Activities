#pragma once

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <unistd.h>

class Timer
{
  private:
    static Timer* instance;
    int           fd;

    volatile bool          running;
    volatile unsigned long elapsed_seconds;
    volatile unsigned int  tick_counter;

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

  public:
    Timer(const std::string& pid);
    ~Timer();

    static void   timer_handler(int signum);
    unsigned long run();
};
