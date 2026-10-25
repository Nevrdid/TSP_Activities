#include "Config.hpp"

#include <fstream>
#include <iostream>

#if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#    error "No filesystem support"
#endif

Config::Config()
{
    // Desktop/native builds do not have the TrimUI system configuration files.
    // Start with usable defaults, then override them when device configuration exists.
    theme.font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

    nlohmann::json j;
    std::ifstream system_file("/mnt/UDISK/system.json");
    if (system_file.is_open()) {
        try {
            system_file >> j;
            theme_path = j.value("theme", "");
            if (theme_path == "../res/")
                theme_path = "/mnt/SDCARD/Themes/CrossMix - OS/";
        } catch (const std::exception& e) {
            std::cerr << "Warning: could not parse /mnt/UDISK/system.json: "
                      << e.what() << std::endl;
            theme_path.clear();
        }
    } else {
        std::cerr << "Info: TrimUI system configuration not found; using default theme settings."
                  << std::endl;
    }

    if (!theme_path.empty()) {
        const std::string theme_config = theme_path + "/config.json";
        if (fs::exists(theme_config) && load_theme(theme_config)) {
            if (theme.font.empty())
                theme.font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
            if (theme.fontColor.count("content_color4"))
                selected_color = theme.fontColor.at("content_color4");
            if (theme.fontColor.count("content_color1"))
                unselect_color = theme.fontColor.at("content_color1");
            if (theme.fontColor.count("nav_color1"))
                title_color = theme.fontColor.at("nav_color1");
            if (theme.fontColor.count("stat_color1"))
                info_color = theme.fontColor.at("stat_color1");
        }
    }

    std::ifstream crossmix_file("/mnt/SDCARD/System/etc/crossmix.json");
    if (crossmix_file.is_open()) {
        try {
            crossmix_file >> j;
            backgrounds_theme = j.value("BACKGROUNDS", backgrounds_theme);
        } catch (const std::exception& e) {
            std::cerr << "Warning: could not parse CrossMix configuration: "
                      << e.what() << std::endl;
        }
    } else {
        std::cerr << "Info: CrossMix configuration not found; using default background settings."
                  << std::endl;
    }
}

bool Config::load_theme(const std::string& filePath)
{
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Warning: could not open theme file " << filePath << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;

        theme.name = j.value("name", "");
        theme.font = j.value("font", theme.font);
        theme.fontAscii = j.value("fontascii", "");

        if (j.contains("fontsize")) {
            for (const auto& [key, value] : j["fontsize"].items())
                theme.fontSize[key] = value.get<int>();
        }

        if (j.contains("fontcolor")) {
            for (const auto& [key, value] : j["fontcolor"].items())
                theme.fontColor[key] = parseColor(value.get<std::string>());
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: error parsing theme JSON: " << e.what() << std::endl;
        return false;
    }
    return true;
}

SDL_Color Config::parseColor(const std::string& colorStr) const
{
    if (colorStr.size() != 9 || colorStr[0] != '#')
        throw std::invalid_argument("Invalid color format: " + colorStr);

    Uint8 a = std::stoi(colorStr.substr(1, 2), nullptr, 16);
    Uint8 r = std::stoi(colorStr.substr(3, 2), nullptr, 16);
    Uint8 g = std::stoi(colorStr.substr(5, 2), nullptr, 16);
    Uint8 b = std::stoi(colorStr.substr(7, 2), nullptr, 16);

    return SDL_Color{r, g, b, a};
}

Config::~Config()
{
}
