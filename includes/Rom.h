#pragma once

#include "DB.h"
#include "GUI.h"

#include <string>

class Rom
{
  private:
    static GUI&    gui;
    static Config& cfg;
    static DB&     db;

    static std::unordered_set<Rom*>     ra_hotkey_roms;
    static std::map<std::string, pid_t> childs;

    void fill_opts();
  public:
    Rom(DB_row row);
    Rom(const std::string& file);
    Rom(const std::string& file, int time);
    ~Rom();

    std::string file;            // #0 in games_datas
    std::string name;            // #1 in games_datas
    int         count = 0;       // #2 in games_datas
    int         time = 0;        // #3 in games_datas
    int         lastsessiontime; // #4 in games_datas (duration in seconds)
    std::string last = "-";      // #5 in games_datas
    int         completed = -1;  // #6 in games_datas
    int         favorite = -1;   // #7 in games_datas

    std::string total_time;
    std::string average_time;
    std::string system;
    std::string image;
    std::string video;
    std::string manual;
    pid_t       pid = -1;
    std::string launcher;

    void update(DB_row row);
    DB_row get_DB_row();
    void   save();

    void  start();
    void  stop();
    int   wait();
    pid_t suspend();

    static void             export_childs_list();
    static std::vector<Rom> getAll(std::vector<Rom> previous_roms);
};
