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

static const std::string completed_names[] = {"All", "Completed", "Not completed"};

class App
{
  private:
    Config cfg;
    GUI    gui;

    TTF_Font* font_big;
    TTF_Font* font_middle;
    TTF_Font* font_tiny;
    TTF_Font* font_mini;

    bool is_running = false;
    bool in_game_detail = false;

    std::vector<Rom> roms_list;
    std::vector<Rom> filtered_roms_list;
    size_t           list_size = 0;
    size_t           selected_index = 0;
    bool             no_list = false;
    int              filter_completed = 0;
    Sort             sort_by = Sort::Last;

    std::vector<std::string> systems;
    size_t                   system_index = 0;

    void sort_roms();
    void filter_roms();

    void switch_completed();
    void handle_inputs();
    void game_list();
    void game_detail();
    void overall_stats();
    bool confirmation_popup(const std::string& message);
    void empty_db();

  public:
    App(const std::string& rom_file = "");
    ~App();

    void run();
};
