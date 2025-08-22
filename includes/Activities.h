#pragma once

#include "DB.h"
#include "GUI.h"
#include "GameRunner.h"
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

enum Sort
{
    Name,
    Time,
    Count,
    Last
};

static const std::string sort_names[] = {"Name", "Time", "Count", "Last"};

static const std::string states_names[] = {"All", "Running", "Completed", "Not completed"};

class Activities
{
  public:
    // Allow main to set these for special GUI modes
  private:
    Config     cfg;
    GUI        gui;
    GameRunner game_runner;

    bool   is_running = false;
    bool   interupted = false;
    bool   in_game_detail = false;
    size_t selected_index = 0;

    bool                                  need_refresh = false;
    std::chrono::steady_clock::time_point refresh_timer_start;
    bool                                  refresh_timer_active = false;

    std::vector<Rom>                        roms_list;
    std::vector<std::vector<Rom>::iterator> filtered_roms_list;
    size_t                                  list_size = 0;

    size_t total_time = 0;
    bool   no_list = false;
    int    filter_state = 0;

    Sort sort_by = Sort::Last;

    std::vector<std::string> systems;
    size_t                   system_index = 0;

    // Auto-scroll (key repeat) management for Up/Down in the list
    bool upHolding = false;   // true while UP is held
    bool downHolding = false; // true while DOWN is held
    // Auto-scroll for Left/Right in detail view
    bool leftHolding = false;                            // true while LEFT is held (next item)
    bool rightHolding = false;                           // true while RIGHT is held (previous item)
    std::chrono::steady_clock::time_point holdStartTime; // time when hold started
    std::chrono::steady_clock::time_point lastRepeatTime; // time of last auto-scroll step
    const int initialRepeatDelayMs = 550;                 // delay before auto-scroll starts (ms)
    const int listRepeatIntervalMs = 80;                  // interval for list scrolling (faster)
    const int detailRepeatIntervalMs = 200;               // interval for detail navigation (slower)

    int  parseArgs(int argc, char** argv);
    void sort_roms();
    void filter_roms();

    Rom* get_rom(const std::string& rom_file = "");
    void switch_completed();
    void handle_inputs();
    void menu(std::vector<Rom>::iterator rom);
    void game_list();
    void game_detail();
    void overall_stats();
    void empty_db();

  public:
    Activities();
    ~Activities();

    void refresh_db(std::string selected_rom_file = "");
    void run(int argc, char** argv);
};
