#pragma once
#include "Rom.h"

#include <Rom.h>
#include <sqlite3.h>
#include <string>
#include <vector>

#define DB_FILE APP_DIR "data/games.db"

class DB
{
  private:
    DB();
    DB(const DB& copy);
    DB& operator=(const DB& copy);

    sqlite3*    db;      // SQLite database connection
    std::string db_file; // SQLite database file

  public:
    ~DB();

    static DB& getInstance()
    {
        static DB instance;
        return instance;
    }

    bool             is_refresh_needed();
    void             save(Rom& rom);
    Rom              load(const std::string& rom_file);
    std::vector<Rom> load(std::vector<Rom> roms);
    void             remove(const std::string& file);
};
