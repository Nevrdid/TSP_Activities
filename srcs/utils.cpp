#include "utils.hpp"

#include <csignal>
#include <fstream>
#include <iostream>

namespace utils
{
std::string getCurrentDateTime()
{
    std::time_t now = std::time(nullptr);
    struct tm*  tm_info = std::localtime(&now);

    if (tm_info == nullptr) {
        std::cerr << "Failed to get current time!" << std::endl;
        return "";
    }
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string sec2hhmmss(int total_seconds)
{
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0')
        << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;

    return oss.str();
}

std::string stringifyTime(int total_seconds)
{
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    std::ostringstream oss;
    if (hours == 0) {
        oss << std::setw(2) << std::setfill('0') << minutes << "' " << std::setw(2)
            << std::setfill('0') << seconds << "\"";

    } else {
        oss << std::setw(2) << std::setfill('0') << hours << "h " << std::setw(2)
            << std::setfill('0') << minutes << "\'";
    }

    return oss.str();
}

pid_t get_pid_of_process(const std::string& command)
{
    std::string pid_command = "pgrep -f '" + command + "'";
    FILE*       pipe = popen(pid_command.c_str(), "r");
    pid_t       pid = 0;
    if (pipe) {
        if (fscanf(pipe, "%d", &pid) != 1)
            pid = 0;
        pclose(pipe);
    }
    return pid;
}

pid_t get_pgid_of_process(pid_t pid)
{
    pid_t pgid = getpgid(pid);
    return pgid;
}

void suspend_process_group(pid_t pgid)
{
    if (pgid > 0)
        kill(-pgid, SIGSTOP);
}

void resume_process_group(pid_t pgid)
{
    if (pgid > 0)
        kill(-pgid, SIGCONT);
}

void kill_process_group(pid_t pgid)
{
    if (pgid > 0)
        kill(-pgid, SIGTERM);
}

// Returns true if /tmp/trimui_inputd/ra_hotkey exists
bool ra_hotkey_exists()
{
    struct stat st;
    return stat("/tmp/trimui_inputd/ra_hotkey", &st) == 0;
}

// Remove the ra_hotkey file if present
void remove_ra_hotkey()
{
    unlink("/tmp/trimui_inputd/ra_hotkey");
}

// Restore the ra_hotkey file (empty file) if parent dir exists
void restore_ra_hotkey()
{
    // Ensure directory exists; if not, do nothing
    struct stat st;
    if (stat("/tmp/trimui_inputd", &st) != 0 || !S_ISDIR(st.st_mode))
        return;
    int fd = open("/tmp/trimui_inputd/ra_hotkey", O_CREAT | O_WRONLY, 0644);
    if (fd >= 0)
        close(fd);
}

std::string shorten_file_path(fs::path filepath, std::string unknown_part)
{
    fs::path selected_rom_path(filepath);

    if (fs::exists(filepath)) {
        if (!unknown_part.empty())
            return fs::canonical(selected_rom_path).string() + "/" + unknown_part;
        return fs::canonical(selected_rom_path).string();
    } else if (filepath.string() == "/") {
        return unknown_part;
    } else {
        unknown_part = filepath.filename().string();
        return shorten_file_path(filepath.parent_path(), unknown_part);
    }
}

std::vector<std::string> get_launchers(const std::string& system)
{
    std::vector<std::string> launchers;
    std::ifstream            sys_cfg2("/mnt/SDCARD/Emus/" + system + "/config.json");
    if (!sys_cfg2.is_open())
        return launchers;

    try {
        nlohmann::json j;
        sys_cfg2 >> j;
        if (!j.is_object() || !j.contains("launchlist") || !j["launchlist"].is_array())
            return launchers;

        for (const auto& value : j["launchlist"]) {
            if (!value.is_object() || !value.contains("launch") || !value["launch"].is_string() ||
                !value.contains("name") || !value["name"].is_string())
                continue;

            if (value["launch"].get<std::string>().size() > 1) {
                const std::string name = value["name"].get<std::string>();
                if (!name.empty())
                    launchers.push_back(name);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "ActivitiesApp: Could not read launchers for " << system << ": "
                  << e.what() << std::endl;
    }
    return launchers;
}

std::string get_launcher(const std::string& system, const std::string& romName)
{
    std::string   ret;
    std::ifstream game_cfg("/mnt/SDCARD/Roms/" + system + "/.games_config/" + romName + ".cfg");
    if (game_cfg.is_open() && !game_cfg.fail()) {
        std::string line;
        std::getline(game_cfg, line, '=');
        std::getline(game_cfg, line, '=');
        game_cfg.close();
        ret = line;
    } else {

        std::ifstream sys_cfg("/mnt/SDCARD/Emus/" + system + "/launchers.cfg");
        if (sys_cfg.is_open() && !sys_cfg.fail()) {
            std::string line;
            std::getline(sys_cfg, line, '=');
            std::getline(sys_cfg, line, '=');
            sys_cfg.close();
            ret = line;
        } else {
            std::ifstream sys_cfg2("/mnt/SDCARD/Emus/" + system + "/config.json");
            if (sys_cfg2.is_open()) {
                try {
                    nlohmann::json j;
                    sys_cfg2 >> j;

                    if (j.is_object() && j.contains("launchlist") && j["launchlist"].is_array()) {
                        for (const auto& value : j["launchlist"]) {
                            if (!value.is_object() || !value.contains("launch") ||
                                !value["launch"].is_string() || !value.contains("name") ||
                                !value["name"].is_string())
                                continue;
                            if (value["launch"].get<std::string>().size() > 1) {
                                ret = value["name"].get<std::string>();
                                break;
                            }
                        }
                    } else if (j.is_object() && j.contains("launch.sh") &&
                               j["launch.sh"].is_string()) {
                        ret = j["launch.sh"].get<std::string>();
                    }
                } catch (const std::exception& e) {
                    std::cerr << "ActivitiesApp: Could not read launcher configuration for "
                              << system << ": " << e.what() << std::endl;
                }
            }
        }
    }
    if (ret.empty())
        return "-";
    if (ret.back() == '\n')
        ret.pop_back();
    return ret;
}

void set_launcher(
    const std::string& system, const std::string& romName, const std::string& launcher)
{
    std::string cfg_folder("/mnt/SDCARD/Roms/" + system + "/.games_config");
    if (!fs::exists(cfg_folder))
        fs::create_directory(cfg_folder);
    std::ofstream game_cfg(cfg_folder + "/" + romName + ".cfg");
    if (!game_cfg.fail()) {
        game_cfg << "launcher=" << launcher << std::endl;
        game_cfg.close();
    }
    return;
}

std::vector<std::string> get_directory_content(fs::path location, bool hide_hidden)
{
    std::vector<std::string> content;
    std::error_code ec;
    if (!fs::exists(location, ec) || ec || !fs::is_directory(location, ec) || ec) {
        if (ec)
            std::cerr << "ActivitiesApp: Could not inspect directory " << location << ": "
                      << ec.message() << std::endl;
        return content;
    }

    content.push_back("..");
    fs::directory_iterator entry(location, ec);
    const fs::directory_iterator end;
    while (!ec && entry != end) {
        const std::string filename = entry->path().filename().string();
        if (!hide_hidden || filename.empty() || filename[0] != '.')
            content.push_back(filename);
        entry.increment(ec);
    }
    if (ec)
        std::cerr << "ActivitiesApp: Could not list directory " << location << ": "
                  << ec.message() << std::endl;
    return content;
}

} // namespace utils
