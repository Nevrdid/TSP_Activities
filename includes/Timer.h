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
    int           fd;
    unsigned long elapsed_seconds;
    unsigned int  tick_counter;
    volatile bool running;
    static Timer* instance;

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

  public:
    Timer(const std::string& pid);
    ~Timer();

    static void   timer_handler(int signum);
    unsigned long run();
};
