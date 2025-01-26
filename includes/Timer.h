#pragma once

#include "utils.h"

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
    static Timer*          instance;
    int                    fd = -1;
    volatile bool          running = false;
    volatile unsigned long elapsed_seconds = 0;
    volatile unsigned int  tick_counter = 0;

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

  public:
    Timer(const std::string& pid);
    ~Timer();

    static void   timer_handler(int signum);
    unsigned long run();
};
