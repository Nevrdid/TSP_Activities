#pragma once
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

#define DB_FILE "./assets/games.db"

typedef struct
{
    unsigned long crc;   // #1 in games_datas
    std::string   file;  // #2 in games_datas
    std::string   name;  // #3 in games_datas
    int           count; // #4 in games_datas
    int           time;  // #5 in games_datas
    std::string   last;  // #6 in games_datas

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
