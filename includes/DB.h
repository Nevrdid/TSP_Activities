#pragma once
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

#define DB_FILE APP_DIR "assets/games.db"
#define DEFAULT_IMAGE APP_DIR "/assets/placeholder.png"

typedef struct
{
    unsigned long crc;   // #0 in games_datas
    std::string   file;  // #1 in games_datas
    std::string   name;  // #2 in games_datas
    int           count; // #3 in games_datas
    int           time;  // #4 in games_datas
    std::string   last;  // #5 in games_datas

    std::string total_time;
    std::string average_time;
    std::string image;
    std::string system;
} Rom;

class DB
{
  private:
    sqlite3*    db;      // SQLite database connection
    std::string db_file; // SQLite database file

  public:
    DB();
    ~DB();
    Rom              save(const std::string& rom_file, int elapsed_time);
    Rom              load(const std::string& rom_file);
    std::vector<Rom> load_all();
    void             remove(const std::string& file);
};
