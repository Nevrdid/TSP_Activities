#include "DB.h"

#include "utils.h"

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

Rom DB::save(Rom& rom, int time)
{
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
    sqlite3_bind_text(stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) { // Record exists
        // Existing values from DB
        int         db_count = sqlite3_column_int(stmt, 2);
        int         db_time = sqlite3_column_int(stmt, 3);
        std::string db_last =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        int db_completed = sqlite3_column_int(stmt, 6);

        // If time == 0, we're likely toggling "completed" without adding a session.
        // Preserve existing counters and last if no session length is provided.
        if (time == 0) {
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

        // Only skip update for too-short sessions when we actually recorded a session.
        if (time > 0 && time < 5) {
            sqlite3_finalize(stmt);
            // Populate derived/display fields before returning to avoid blank UI data
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

            rom.video = std::regex_replace(rom.file, img_pattern, R"(/Videos/$1)") + "/" +
                        rom.name + ".mp4";
            if (!fs::exists(rom.video))
                rom.video = "";

            rom.manual = std::regex_replace(rom.file, img_pattern, R"(/Manuals/$1)") + "/" +
                         rom.name + ".pdf";
            if (!fs::exists(rom.manual))
                rom.manual = "";

            rom.system = std::regex_replace(rom.file, sys_pattern, R"($1)");
            return rom;
        }

        std::string   update_query = "UPDATE games_datas SET name = ?, count = ?, time = ?, "
                                     " lastsessiontime = ?, last = ?, completed = ? WHERE file = ?";
        sqlite3_stmt* update_stmt;
        if (sqlite3_prepare_v2(db, update_query.c_str(), -1, &update_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing UPDATE query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            rom.total_time = utils::stringifyTime(rom.time);
            rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
            return rom;
        }

        sqlite3_bind_text(update_stmt, 1, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 2, rom.count);
        sqlite3_bind_int(update_stmt, 3, rom.time);
        sqlite3_bind_int(update_stmt, 4, rom.lastsessiontime);
        sqlite3_bind_text(update_stmt, 5, rom.last.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(update_stmt, 6, rom.completed);
        sqlite3_bind_text(update_stmt, 7, rom.file.c_str(), -1, SQLITE_STATIC);

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

        // When no existing record: if this call only toggles "completed" (time==0),
        // we still want to create an entry; otherwise keep the original guard.
        if (!(time == 0 && rom.completed != -1)) {
            // Do not record if last session is too short
            if (time < 5) {
                sqlite3_finalize(stmt);

                rom.total_time = utils::stringifyTime(rom.time);
                rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
                return rom;
            }
        }
        std::string   insert_query = "INSERT INTO games_datas (file, name, count, time, "
                                     "lastsessiontime, last, completed) VALUES (?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* insert_stmt;

        if (sqlite3_prepare_v2(db, insert_query.c_str(), -1, &insert_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing INSERT query: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            rom.total_time = utils::stringifyTime(rom.time);
            rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
            return rom;
        }

        sqlite3_bind_text(insert_stmt, 1, rom.file.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, rom.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert_stmt, 3, rom.count);
        sqlite3_bind_int(insert_stmt, 4, rom.time);
        sqlite3_bind_int(insert_stmt, 5, rom.lastsessiontime);
        sqlite3_bind_text(insert_stmt, 6, (time == 0 ? std::string("-") : rom.last).c_str(), -1,
            SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_stmt, 7, (rom.completed == -1 ? 0 : rom.completed));

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
    return rom;
}

Rom DB::save(const std::string& file, int time)
{
    Rom rom;
    rom.file = utils::shorten_file_path(file);

    fs::path filepath(file);
    rom.name = filepath.stem();
    rom.count = time ? 1 : 0;
    rom.time = time;
    rom.last = time ? utils::getCurrentDateTime() : "-";
    rom.completed = -1;
    rom.lastsessiontime = 0;
    rom.pid = -1;
    return save(rom, time);
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
        rom.lastsessiontime = sqlite3_column_int(stmt, 4);
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        rom.completed = sqlite3_column_int(stmt, 6);
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
        rom = save(file);
    }

    sqlite3_finalize(stmt);
    return rom;
}

std::vector<Rom> DB::load(std::vector<Rom> roms)
{
    std::cout << "DB: Loading all roms." << std::endl;

    if (!db) {
        std::cerr << "No connection to SQLite database." << std::endl;
        return roms;
    }

    std::string   query = "SELECT * FROM games_datas ORDER_BY last DESC";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SELECT query: " << sqlite3_errmsg(db) << std::endl;
        return roms;
    }

    bool is_reload = !roms.empty();
    auto it = roms.begin();
    size_t i = 0;
    // input roms when reloading and db should have exact same
    // roms as when can't add roms out of a activity app start ( no reload)
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Rom rom;
        rom.file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        rom.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        rom.count = sqlite3_column_int(stmt, 2);
        rom.time = sqlite3_column_int(stmt, 3);
        rom.lastsessiontime = sqlite3_column_int(stmt, 4);
        rom.last = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        rom.completed = sqlite3_column_int(stmt, 6);

        rom.total_time = utils::stringifyTime(rom.time);
        rom.average_time = utils::stringifyTime(rom.count ? rom.time / rom.count : 0);
        if (!is_reload) {
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
            rom.pid = -1;
            roms.push_back(rom);
        } else {
            rom.image = it->image;
            rom.video = it->video;
            rom.manual = it->manual;
            rom.system = it->system;
            rom.pid = it->pid;
            roms[i++] = rom;        
            it++;
        }

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
