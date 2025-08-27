#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

#define DB_FILE APP_DIR "data/games.db"

class Rom;

struct DB_row
{
    std::string file;
    std::string name;
    int         count;
    int         time;
    int         lastsessiontime;
    std::string last;
    int         completed;
    int         favorite;
};

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

    bool is_refresh_needed();

    void                save(DB_row entry);
    std::vector<DB_row> load();
    DB_row              load(const std::string& file);

    void remove(const std::string& file);
};
