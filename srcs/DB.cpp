#include "DB.h"

#include <iostream>

DB::DB()
    : db(nullptr)
{

    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
    }
    std::string query = "CREATE TABLE IF NOT EXISTS games_datas ("
                        "file TEXT PRIMARY KEY NOT NULL,"
                        "name TEXT NOT NULL,"
                        "count INTEGER NOT NULL,"
                        "time INTEGER NOT NULL,"
                        "lastsessiontime INTEGER NOT NULL,"
                        "last TEXT NOT NULL,"
                        "completed INTEGER NOT NULL,"
                        "favorite INTEGER NOT NULL"
                        ")";
    char*       err_msg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Error creating table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}

DB::~DB()
{
    if (db) {
        sqlite3_close(db);
    }
}

bool DB::is_refresh_needed()
{
    static int data_version = 1;

    bool is_needed = false;

    std::string   query = "PRAGMA data_version";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing PRAGMA query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int current_data_version = sqlite3_column_int(stmt, 0);

        if (current_data_version != data_version) {
            data_version = current_data_version;
            is_needed = true;
        }
    } else {
        std::cerr << "Error executing PRAGMA query" << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);

    return is_needed;
}

void DB::save(DB_row entry)
{
    DB_row previous_entry = load(entry.file);
    if (!previous_entry.file.empty()) {
        if (entry.time == 0) {
            entry.count = previous_entry.count;
            entry.time = previous_entry.time;
            entry.lastsessiontime = 0;
            entry.last = previous_entry.last;
        } else {
            entry.count += previous_entry.count;
            entry.lastsessiontime = entry.time; // duration of the last session
            entry.time += previous_entry.time;
        }

        if (entry.completed == -1)
            entry.completed = previous_entry.completed;
        if (entry.favorite == -1)
            entry.favorite = previous_entry.completed;
    }

    std::string update_query =
        "UPDATE games_datas SET name = ?, count = ?, time = ?, "
        " lastsessiontime = ?, last = ?, completed = ?, favorite = ? WHERE file = ?";
    sqlite3_stmt* update_stmt;
    if (sqlite3_prepare_v2(db, update_query.c_str(), -1, &update_stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing UPDATE query: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(update_stmt, 1, entry.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(update_stmt, 2, entry.count);
    sqlite3_bind_int(update_stmt, 3, entry.time);
    sqlite3_bind_int(update_stmt, 4, entry.lastsessiontime);
    sqlite3_bind_text(update_stmt, 5, entry.last.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(update_stmt, 6, entry.completed);
    sqlite3_bind_int(update_stmt, 7, entry.favorite);
    sqlite3_bind_text(update_stmt, 8, entry.file.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(update_stmt) != SQLITE_DONE)
        std::cerr << "Error updating record: " << sqlite3_errmsg(db) << std::endl;
    else
        std::cout << "Record updated for rom: " << entry.name << std::endl;

    sqlite3_finalize(update_stmt);
}

DB_row DB::load(const std::string& file)
{
    DB_row ret;
    std::cout << "DB: Loading " << file << " details." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return ret;
    }

    std::string   query = "SELECT * FROM games_datas WHERE file = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return ret;
    }

    sqlite3_bind_text(stmt, 1, file.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        std::cout << "Entry exist in database." << std::endl;
        ret.file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        ret.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        ret.count = sqlite3_column_int(stmt, 2);
        ret.time = sqlite3_column_int(stmt, 3);
        ret.lastsessiontime = sqlite3_column_int(stmt, 4);
        ret.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        ret.completed = sqlite3_column_int(stmt, 6);
        ret.favorite = sqlite3_column_int(stmt, 7);
        std::cout << "Entry loaded." << std::endl;
    } else {
        std::cerr << "No record found for rom: " << file << std::endl;
    }

    sqlite3_finalize(stmt);
    return ret;
}

std::vector<DB_row> DB::load()
{
    std::vector<DB_row> table;

    std::cout << "DB: Loading all roms." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return table;
    }

    std::string   query = "SELECT * FROM games_datas ORDER BY last DESC";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return table;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DB_row row;
        row.file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        row.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        row.count = sqlite3_column_int(stmt, 2);
        row.time = sqlite3_column_int(stmt, 3);
        row.lastsessiontime = sqlite3_column_int(stmt, 4);
        row.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        row.completed = sqlite3_column_int(stmt, 6);
        row.favorite = sqlite3_column_int(stmt, 7);
        table.push_back(row);
    }
    sqlite3_finalize(stmt);
    return table;
}

void DB::remove(const std::string& file)
{
    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return;
    }

    std::string   query = "DELETE FROM games_datas WHERE file = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing DELETE query: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, file.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        std::cerr << "Error deleting record: " << sqlite3_errmsg(db) << std::endl;
    else
        std::cout << "Record deleted for rom: " << file << std::endl;

    sqlite3_finalize(stmt);
}
