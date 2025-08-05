#pragma once

#include "DB.h"
#include "GUI.h"
#include "utils.h"

#include <algorithm>
#include <set>
#include <string>
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
    int selected_index = 0;
    bool in_game_detail = false;
    Sort sort_by = Sort::Last;
    void filter_roms();
  private:
    Config cfg;
    GUI    gui;

    bool is_running = false;
    bool need_refresh = false;

    std::vector<Rom> roms_list;
    std::vector<Rom> filtered_roms_list;
    size_t           list_size = 0;
    size_t           total_time = 0;
    bool             no_list = false;
    int              filter_state = 0;

    std::vector<std::string> systems;
    size_t                   system_index = 0;

    void sort_roms();

    void switch_completed();
    void set_pid(pid_t pid);
    void handle_inputs();
    void game_list();
    void game_detail();
    void overall_stats();
    void empty_db();

  public:
    Activities(const std::string& rom_file = "");
    ~Activities();

    void run();
};
