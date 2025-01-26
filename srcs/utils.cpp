#include "utils.h"

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
    oss << std::put_time(tm_info, "%Y%m%d.%H%M%S");
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

std::string stringifyDate(const std::string& date)
{
    if (date == "-")
        return date;
    std::string year = date.substr(0, 4);
    std::string month = date.substr(4, 2);
    std::string day = date.substr(6, 2);
    std::string hour = date.substr(9, 2);
    std::string minute = date.substr(11, 2);
    std::string second = date.substr(13, 2);

    return year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second;
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

} // namespace utils
