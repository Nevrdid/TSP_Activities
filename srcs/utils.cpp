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
unsigned long calculateCRC32(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error opening file!" << std::endl;
        return 0;
    }

    unsigned long crc = 0xffffffff;

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        crc = crc32(crc, reinterpret_cast<const unsigned char*>(buffer), file.gcount());
    }

    crc ^= 0xffffffff;
    return crc;
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
} // namespace utils
