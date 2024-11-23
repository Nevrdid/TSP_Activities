#pragma once
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>

namespace utils
{
std::string   getCurrentDateTime();
std::string   sec2hhmmss(int total_seconds);
} // namespace utils
