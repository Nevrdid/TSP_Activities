#pragma once
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

#define DB_FILE APP_DIR "data/games.db"

typedef struct
{
    std::string file;     // #0 in games_datas
    std::string name;     // #1 in games_datas
    int         count;    // #2 in games_datas
    int         time;     // #3 in games_datas
    std::string last;     // #4 in games_datas
    int         completed; // #5 in games_datas

    std::string total_time;
    std::string average_time;
    std::string system;
    std::string image;
    std::string video;
    std::string manual;
} Rom;

class DB
{
  private:
    sqlite3*    db;      // SQLite database connection
    std::string db_file; // SQLite database file

  public:
    DB();
    ~DB();
    Rom              save(const std::string& rom_file, int elapsed_time = 0, int completed = -1);
    Rom              load(const std::string& rom_file);
    std::vector<Rom> load_all();
    void             remove(const std::string& file);
};
