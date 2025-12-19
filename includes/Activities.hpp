#pragma once

#include "DB.hpp"
#include "GUI.hpp"
#include "Rom.hpp"

#include <chrono>
#include <cstdio>
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

static const char help[] = {
    "activities usage:\n"
    "\tactivities [option...]*\n"
    "Options:\n"
    "  -h\tDisplay help text\n"
    "  -D\tStart the gui in details mode instead of list\n"
    "  -s\tSet the initial sort method. Default: (name,time,count,\e[0mlast\e[1m)\n"
    "  -r\tReverse the initial sort order. "
    "  -S\tSet the initial system to filter. (\e[1mall\e[0m,gba,psp,fc...)\n"
    "  -c\tSet the initial completed filter (\e[1mall\e[1m,on,off)\n"
    "  -f\tSet the initial romFile to display. default: first of the filtered and sorted list. "
    "\n"};



enum FilterState
{
    All = -1, // Show only match
    Unmatch,  // Show only unmatchs
    Match     // Show both
};

struct FiltersStates
{
    int running = FilterState::All;
    int favorites = FilterState::All;
    int completed = FilterState::All;
};

class Activities
{
  private:
    Activities();
    Activities(const Activities& copy);
    Activities& operator=(const Activities& copy);

    Config& cfg;
    GUI&    gui;
    DB&     db;

    bool   is_running = false;
    bool   in_game_detail = false;
    bool   auto_resume_enabled = true;
    size_t selected_index = 0;

    std::vector<Rom>&                       roms_list = Rom::get();
    std::vector<std::vector<Rom>::iterator> filtered_roms_list;
    size_t                                  list_size = 0;

    size_t        total_time = 0;
    FiltersStates filters_states = {FilterState::All};

    Sort sort_by = Sort::Last;
    bool reverse_sort = false;

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

    int parseArgs(int argc, char** argv);

    void handle_game_return(int wait_status);
    void handle_inputs();

    Rom* get_rom(const std::string& rom_file = "");

    void sort_roms();
    void filter_roms();

    MenuResult switch_filter(const std::string& label, int& state);
    MenuResult sort_menu();
    MenuResult filters_menu();
    void       global_menu();
    void       game_menu(std::vector<Rom>::iterator rom);

    void start_external(const std::string& command);
    void game_list();
    void game_detail();
    void overall_stats();
    void empty_db();

    void refresh_db(std::string selected_rom_file = "");
    void auto_resume();

  public:
    ~Activities();

    static Activities& getInstance()
    {
        static Activities instance;
        return instance;
    }

    void run(int argc, char** argv);
};
