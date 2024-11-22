#pragma once

#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using std::string;

class Timer
{
  public:
    Timer(const string& file, const string& pid);
    ~Timer();

    unsigned long run();

  private:
    int           get_target_state();
    std::string   get_command_from_pid(const std::string& pid);
    string        target_rom;
    string        target_pid;
    unsigned long elapsed_seconds;
};
