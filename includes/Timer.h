#pragma once

#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <unistd.h>

using std::string;

class Timer
{
  public:
    Timer(const string& pid);
    ~Timer();

    unsigned long run();

  private:
    static Timer* instance; // Singleton instance to access from signal handler
    int    fd;
    unsigned long elapsed_seconds;
    volatile bool running;

    static void timer_handler(int signum);
};
