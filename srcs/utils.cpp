#include "utils.h"

#include <fcntl.h>
#include <sys/stat.h>

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
        fscanf(pipe, "%d", &pid);
        fclose(pipe);
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

} // namespace utils
