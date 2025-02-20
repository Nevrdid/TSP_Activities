#include "DB.h"

#include "utils.h"

#include <regex>

static std::regex img_pattern = std::regex(R"(\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"
static std::regex sys_pattern =
    std::regex(R"(.*\/Roms\/([^\/]+).*)"); // Matches "/Roms/<subfolder>"

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
                        "last TEXT NOT NULL,"
                        "completed INTEGER NOT NULL"
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

Rom DB::save(const std::string& file, int time, int completed)
{
    Rom rom;
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
    std::filesystem::path filepath(file);

    if (std::filesystem::exists(filepath))
        rom.file = std::filesystem::canonical(filepath);
    else
        rom.file = filepath;
    rom.name = filepath.stem();
    rom.count = time ? 1 : 0;
    rom.time = time;
    rom.last = time ? utils::getCurrentDateTime() : "-";
    rom.completed = completed == -1 ? 0 : completed;

    sqlite3_bind_text(stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) { // Record exists
        rom.count += sqlite3_column_int(stmt, 2);
        if (rom.time == 0)
            rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        rom.time += sqlite3_column_int(stmt, 3);
        rom.completed = completed == -1 ? sqlite3_column_int(stmt, 5) : completed;

        std::string   update_query = "UPDATE games_datas SET name = ?, count = ?, time = ?, "
                                     " last = ?, completed = ? WHERE file = ?";
        sqlite3_stmt* update_stmt;
        if (sqlite3_prepare_v2(db, update_query.c_str(), -1, &update_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing UPDATE query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return rom;
        }

        sqlite3_bind_text(update_stmt, 1, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 2, rom.count);
        sqlite3_bind_int(update_stmt, 3, rom.time);
        sqlite3_bind_text(update_stmt, 4, rom.last.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 5, rom.completed);

        sqlite3_bind_text(update_stmt, 6, rom.file.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(update_stmt) != SQLITE_DONE) {
            std::cerr << "Error updating record: " << sqlite3_errmsg(db) << std::endl;
        } else {
            std::cout << "Record updated for rom: " << rom.name << std::endl;
        }

        sqlite3_finalize(update_stmt);
    } else if (result == SQLITE_DONE) {
        std::string   insert_query = "INSERT INTO games_datas (file, name, count, time, "
                                     "last, completed) VALUES (?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* insert_stmt;
        if (sqlite3_prepare_v2(db, insert_query.c_str(), -1, &insert_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing INSERT query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return rom;
        }

        sqlite3_bind_text(insert_stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert_stmt, 3, rom.count);
        sqlite3_bind_int(insert_stmt, 4, rom.time);
        sqlite3_bind_text(insert_stmt, 5, rom.last.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert_stmt, 6, rom.completed);

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

    rom.last = utils::stringifyDate(rom.last);

    rom.image = std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)") + "/" + rom.name + ".png";
    if (!std::filesystem::exists(rom.image))
        rom.image = "";

    rom.video =
        std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
    if (!std::filesystem::exists(rom.video))
        rom.video = "";

    rom.manual =
        std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
    if (!std::filesystem::exists(rom.manual))
        rom.manual = "";

    rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
    rom.pid = -1;
    return rom;
}

Rom DB::load(const std::string& file)
{
    Rom rom;
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
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        rom.last = utils::stringifyDate(rom.last);
        rom.completed = sqlite3_column_int(stmt, 5);
        std::cout << "Entry loaded." << std::endl;

        rom.total_time = utils::stringifyTime(rom.time);

        rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);

        rom.image =
            std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)") + "/" + rom.name + ".png";
        if (!std::filesystem::exists(rom.image))
            rom.image = "";

        rom.video =
            std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
        if (!std::filesystem::exists(rom.video))
            rom.video = "";

        rom.manual =
            std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
        if (!std::filesystem::exists(rom.manual))
            rom.manual = "";

        rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
        rom.pid = -1;
    } else if (result == SQLITE_DONE) {
        return save(file);
        std::cerr << "No record found for rom: " << file << std::endl;
    }

    sqlite3_finalize(stmt);
    return rom;
}

std::vector<Rom> DB::load()
{
    std::vector<Rom> roms;
    std::cout << "DB: Loading all roms." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return roms;
    }

    std::string   query = "SELECT * FROM games_datas";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return roms;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Rom rom;
        rom.file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        rom.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        rom.count = sqlite3_column_int(stmt, 2);
        rom.time = sqlite3_column_int(stmt, 3);
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        rom.last = utils::stringifyDate(rom.last);
        rom.completed = sqlite3_column_int(stmt, 5);

        rom.total_time = utils::stringifyTime(rom.time);

        rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);

        rom.image =
            std::regex_replace(rom.file, img_pattern, R"(/Imgs/$1)") + "/" + rom.name + ".png";
        if (!std::filesystem::exists(rom.image))
            rom.image = "";

        rom.video =
            std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" + rom.name + ".mp4";
        if (!std::filesystem::exists(rom.video))
            rom.video = "";

        rom.manual =
            std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" + rom.name + ".pdf";
        if (!std::filesystem::exists(rom.manual))
            rom.manual = "";

        rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
        rom.pid = -1;

        roms.push_back(rom);
    }

    sqlite3_finalize(stmt);
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
