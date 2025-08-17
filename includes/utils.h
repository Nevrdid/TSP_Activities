#pragma once
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <string>
#include <unistd.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>

namespace utils
{
std::string getCurrentDateTime();
std::string sec2hhmmss(int total_seconds);
std::string stringifyTime(int total_seconds);
std::string stringifyDate(const std::string& date);
pid_t get_pid_of_process(const std::string& command);
pid_t get_pgid_of_process(pid_t pid);
void suspend_process_group(pid_t pgid);
void resume_process_group(pid_t pgid);
void kill_process_group(pid_t pgid);
int get_process_status(int fd);
// Hotkey file helpers
bool ra_hotkey_exists();
void remove_ra_hotkey();
void restore_ra_hotkey();
} // namespace utils
