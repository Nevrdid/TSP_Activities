#include "DB.h"

#include "utils.h"

#include <iostream>
#include <regex>

static std::regex img_pattern = std::regex(R"(\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"
static std::regex sys_pattern =
    std::regex(R"(.*\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"
// Matches "/Best/<subfolder>" (for alternate library root)
static std::regex best_pattern = std::regex(R"(\/Best\/([^\/]+).*)");

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
                        "lastsessiontime INTEGER NOT NULL DEFAULT 0,"
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
    static int data_version = -1;

    bool is_needed = false;

    std::string   query = "PRAGMA data_version";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing PRAGMA query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int current_data_version = sqlite3_column_int(stmt, 0);
        // std::cout << "Current data_version: " << current_data_version << std::endl;

        if (current_data_version != data_version) {
            // std::cout << "Data version changed from " << data_version << " to " <<
            // current_data_version << ". Refresh needed!" << std::endl;
            data_version = current_data_version;
            is_needed = true;
        } else {
            // std::cout << "Data version is still " << data_version << ". No refresh needed." <<
            // std::endl;
        }
    } else {
        std::cerr << "Error executing PRAGMA query or no row returned: " << sqlite3_errmsg(db)
                  << std::endl;
    }

    sqlite3_finalize(stmt);

    return is_needed;
}

void DB::save(Rom& rom)
{
    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return;
    }

    std::string   query = "SELECT * FROM games_datas WHERE file = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    sqlite3_bind_text(stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) { // Record exists
        // Existing values from DB
        int         db_count = sqlite3_column_int(stmt, 2);
        int         db_time = sqlite3_column_int(stmt, 3);
        std::string db_last =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        int db_completed = sqlite3_column_int(stmt, 6);
        int db_favorite = sqlite3_column_int(stmt, 7);

        // If time == 0, we're likely toggling "completed" without adding a session.
        // Preserve existing counters and last if no session length is provided.
        if (rom.time == 0) {
            rom.count = db_count;
            rom.time = db_time;
            rom.lastsessiontime = 0;
            rom.last = db_last;
        } else {
            rom.count += db_count;
            rom.lastsessiontime = rom.time; // duration of the last session
            rom.time += db_time;
        }

        if (rom.completed == -1)
            rom.completed = db_completed;
        if (rom.favorite == -1)
            rom.favorite = db_favorite;

        std::string update_query =
            "UPDATE games_datas SET name = ?, count = ?, time = ?, "
            " lastsessiontime = ?, last = ?, completed = ?, favorite = ? WHERE file = ?";
        sqlite3_stmt* update_stmt;
        if (sqlite3_prepare_v2(db, update_query.c_str(), -1, &update_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing UPDATE query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            rom.total_time = utils::stringifyTime(rom.time);
            rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
            return;
        }

        sqlite3_bind_text(update_stmt, 1, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 2, rom.count);
        sqlite3_bind_int(update_stmt, 3, rom.time);
        sqlite3_bind_int(update_stmt, 4, rom.lastsessiontime);
        sqlite3_bind_text(update_stmt, 5, rom.last.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 6, rom.completed);
        sqlite3_bind_int(update_stmt, 7, rom.favorite);
        sqlite3_bind_text(update_stmt, 8, rom.file.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(update_stmt) != SQLITE_DONE) {
            std::cerr << "Error updating record: " << sqlite3_errmsg(db) << std::endl;
        } else {
            std::cout << "Record updated for rom: " << rom.name << std::endl;
        }

        sqlite3_finalize(update_stmt);

        std::string imgBase;
        if (std::regex_search(rom.file, best_pattern))
            imgBase = std::regex_replace(rom.file, best_pattern, R"(/Best/$1/Imgs)");
        else
            imgBase = std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)");

        rom.image = imgBase + "/" + rom.name + ".png";
        if (!fs::exists(rom.image))
            rom.image = "";

        rom.video =
            std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
        if (!fs::exists(rom.video))
            rom.video = "";

        rom.manual =
            std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
        if (!fs::exists(rom.manual))
            rom.manual = "";

        rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
    } else if (result == SQLITE_DONE) {
        // Initialize ROM metadata even if we don't save to database

        std::string imgBase;
        if (std::regex_search(rom.file, best_pattern))
            imgBase = std::regex_replace(rom.file, best_pattern, R"(/Best/$1/Imgs)");
        else
            imgBase = std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)");

        rom.image = imgBase + "/" + rom.name + ".png";
        if (!fs::exists(rom.image))
            rom.image = "";

        rom.video =
            std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
        if (!fs::exists(rom.video))
            rom.video = "";

        rom.manual =
            std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
        if (!fs::exists(rom.manual))
            rom.manual = "";

        rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");

        std::string insert_query =
            "INSERT INTO games_datas (file, name, count, time, "
            "lastsessiontime, last, completed, favorite) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* insert_stmt;

        if (sqlite3_prepare_v2(db, insert_query.c_str(), -1, &insert_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing INSERT query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            rom.total_time = utils::stringifyTime(rom.time);
            rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
            return;
        }

        sqlite3_bind_text(insert_stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert_stmt, 3, rom.count);
        sqlite3_bind_int(insert_stmt, 4, rom.time);
        sqlite3_bind_int(insert_stmt, 5, rom.lastsessiontime);
        sqlite3_bind_text(insert_stmt, 6, (rom.time == 0 ? std::string("-") : rom.last).c_str(), -1,
            SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_stmt, 7, (rom.completed == -1 ? 0 : rom.completed));
        sqlite3_bind_int(insert_stmt, 8, (rom.favorite == -1 ? 0 : rom.favorite));

        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            std::cerr << "Error inserting record: " << sqlite3_errmsg(db) << std::endl;
        } else {
            std::cout << "New record added for rom: " << rom.name << std::endl;
        }
        sqlite3_finalize(insert_stmt);
    }

    sqlite3_finalize(stmt);

    rom.total_time = utils::stringifyTime(rom.time);
    rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
}

Rom DB::load(const std::string& file)
{
    Rom rom(file);
    std::cout << "DB: Loading " << file << " details." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return rom;
    }

    std::string   query = "SELECT * FROM games_datas WHERE file = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return rom;
    }

    sqlite3_bind_text(stmt, 1, file.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        std::cout << "Entry exist in database." << std::endl;
        rom.file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        rom.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        rom.count = sqlite3_column_int(stmt, 2);
        rom.time = sqlite3_column_int(stmt, 3);
        rom.lastsessiontime = sqlite3_column_int(stmt, 4);
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        rom.completed = sqlite3_column_int(stmt, 6);
        rom.favorite = sqlite3_column_int(stmt, 7);
        std::cout << "Entry loaded." << std::endl;

        rom.total_time = utils::stringifyTime(rom.time);
        rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);

        std::string imgBase;
        if (std::regex_search(rom.file, best_pattern))
            imgBase = std::regex_replace(rom.file, best_pattern, R"(/Best/$1/Imgs)");
        else
            imgBase = std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)");
        rom.image = imgBase + "/" + rom.name + ".png";
        if (!fs::exists(rom.image))
            rom.image = "";

        rom.video =
            std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
        if (!fs::exists(rom.video))
            rom.video = "";

        rom.manual =
            std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
        if (!fs::exists(rom.manual))
            rom.manual = "";

        rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
        rom.pid = -1;
    } else if (result == SQLITE_DONE) {
        std::cerr << "No record found for rom: " << file << std::endl;
        save(rom);
    }

    sqlite3_finalize(stmt);
    return rom;
}

std::vector<Rom> DB::load(std::vector<Rom> previous_roms)
{
    std::cout << "DB: Loading all roms." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return previous_roms;
    }

    std::string   query = "SELECT * FROM games_datas ORDER BY last DESC";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return previous_roms;
    }

    std::vector<Rom> roms;
    bool             is_reload = !previous_roms.empty();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Rom rom(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        rom.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        rom.count = sqlite3_column_int(stmt, 2);
        rom.time = sqlite3_column_int(stmt, 3);
        rom.lastsessiontime = sqlite3_column_int(stmt, 4);
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        rom.completed = sqlite3_column_int(stmt, 6);
        rom.favorite = sqlite3_column_int(stmt, 7);

        rom.total_time = utils::stringifyTime(rom.time);
        rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
        rom.pid = -1;
        if (is_reload) {
            std::vector<Rom>::iterator previous_rom = std::find_if(previous_roms.begin(),
                previous_roms.end(), [&](Rom& r) { return r.file == rom.file; });
            if (previous_rom != previous_roms.end()) {
                rom.pid = previous_rom->pid;
                rom.image = previous_rom->image;
                rom.video = previous_rom->video;
                rom.manual = previous_rom->manual;
                rom.system = previous_rom->system;
            }
        }
        if (rom.pid == -1) {
            std::string imgBase;
            if (std::regex_search(rom.file, best_pattern))
                imgBase = std::regex_replace(rom.file, best_pattern, R"(/Best/$1/Imgs)");
            else
                imgBase = std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)");
            rom.image = imgBase + "/" + rom.name + ".png";

            if (!fs::exists(rom.image))
                rom.image = "";

            rom.video = std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" +
                        rom.name + ".mp4";
            if (!fs::exists(rom.video))
                rom.video = "";

            rom.manual = std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" +
                         rom.name + ".pdf";
            if (!fs::exists(rom.manual))
                rom.manual = "";

            rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
        }
        rom.launcher = utils::get_launcher(rom.system, rom.name);
        roms.push_back(rom);
    }

    sqlite3_finalize(stmt);

    std::cout << "Loaded " << roms.size() << " roms." << std::endl;
    for (const auto& rom : roms) {
        std::cout << "  - " << rom.name << " (" << rom.file << ")" << std::endl;
    }
    return roms;
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

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error deleting record: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Record deleted for rom: " << file << std::endl;
    }

    sqlite3_finalize(stmt);
}
