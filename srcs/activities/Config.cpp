#include "Config.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <set>

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
    nlohmann::json j;
    std::ifstream file("/mnt/UDISK/system.json");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open /mnt/UDISK/system.json" << std::endl;
        exit(1);
    }

    try {
        file >> j;
        theme_path = j.value("theme", "");
    } catch (const std::exception& e) {
        std::cerr << "Error reading /mnt/UDISK/system.json: " << e.what() << std::endl;
        exit(1);
    }

    // Translate the legacy system-theme sentinel to the preferred built-in
    // theme first. Explicit custom theme paths remain authoritative.
    if (theme_path == "../res/" || theme_path.empty())
        theme_path = "/mnt/SDCARD/Themes/TRIMUI YaHei/";

    // Optional per-app selection. Only apply it when system.json requests the
    // legacy/default theme; do not override a user's explicit custom theme.
    const bool default_theme_requested =
        (theme_path == "/mnt/SDCARD/Themes/TRIMUI YaHei/");
    const std::string theme_choice_file = APP_DIR "data/theme.txt";
    std::ifstream theme_choice(theme_choice_file.c_str());
    if (default_theme_requested && theme_choice.is_open()) {
        std::string choice;
        std::getline(theme_choice, choice);
        const std::set<std::string> supported_themes = {
            "TRIMUI YaHei", "TRIMUI Blue", "TRIMUI Energy",
            "TRIMUI HeyDW White", "TRIMUI Retro"
        };
        if (supported_themes.find(choice) != supported_themes.end()) {
            const std::string candidate = "/mnt/SDCARD/Themes/" + choice + "/";
            std::error_code fs_error;
            const fs::path candidate_config = fs::path(candidate) / "config.json";
            const bool candidate_is_installed = fs::is_regular_file(candidate_config, fs_error);
            if (fs_error) {
                std::cerr << "Warning: could not inspect selected theme '" << choice
                          << "': " << fs_error.message()
                          << "; retaining TRIMUI YaHei fallback" << std::endl;
            } else if (candidate_is_installed) {
                theme_path = candidate;
            } else {
                std::cerr << "Warning: selected theme '" << choice
                          << "' is not installed at " << candidate
                          << "; retaining TRIMUI YaHei fallback" << std::endl;
            }
        } else if (!choice.empty()) {
            std::cerr << "Warning: unsupported theme choice '" << choice
                      << "' in " << theme_choice_file << std::endl;
        }
    }

    // Asset paths below are built by appending relative names to theme_path.
    // Normalize the separator here so a valid path without a trailing slash works too.
    if (!theme_path.empty() && theme_path.back() != '/')
        theme_path += '/';

    if (!theme_path.empty()) {
        const std::string theme_config = (fs::path(theme_path) / "config.json").string();
        std::error_code fs_error;
        const bool theme_config_exists = fs::exists(theme_config, fs_error);
        if (fs_error) {
            std::cerr << "Warning: could not inspect theme configuration " << theme_config
                      << ": " << fs_error.message() << std::endl;
        } else if (theme_config_exists && !load_theme(theme_config)) {
            std::cerr << "Warning: theme configuration could not be loaded; using default colors"
                      << std::endl;
        }
    }

    // Keep the built-in colors when a theme omits one of these optional fields.
    const auto apply_theme_color = [this](const std::string& key, SDL_Color& target) {
        const auto color = theme.fontColor.find(key);
        if (color != theme.fontColor.end())
            target = color->second;
    };
    apply_theme_color("content_color4", selected_color);
    apply_theme_color("content_color1", unselect_color);
    apply_theme_color("nav_color1", title_color);
    apply_theme_color("stat_color1", info_color);

    // CrossMix background selection is optional outside a CrossMix installation.
    // The member default ("Default") preserves the normal handheld behavior.
    file.close();
    file.open("/mnt/SDCARD/System/etc/crossmix.json");
    if (file.is_open()) {
        try {
            j.clear();
            file >> j;
            backgrounds_theme = j.value("BACKGROUNDS", "Default");
        } catch (const std::exception& e) {
            std::cerr << "Warning: could not read CrossMix background setting: " << e.what()
                      << "; using Default" << std::endl;
            backgrounds_theme = "Default";
        }
    }
}

bool Config::load_theme(const std::string& filePath)
{
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;

        // Parse fields
        theme.name = j.value("name", "");
        theme.font = j.value("font", "");
        theme.fontAscii = j.value("fontascii", "");

        if (j.contains("fontsize")) {
            for (const auto& [key, value] : j["fontsize"].items()) {
                theme.fontSize[key] = value.get<int>();
            }
        }

        if (j.contains("fontcolor")) {
            for (const auto& [key, value] : j["fontcolor"].items()) {
                theme.fontColor[key] = parseColor(value.get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON file " << filePath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

SDL_Color Config::parseColor(const std::string& colorStr) const
{
    if (colorStr.size() != 9 || colorStr[0] != '#') {
        throw std::invalid_argument("Invalid color format: " + colorStr);
    }

    Uint8 a = std::stoi(colorStr.substr(1, 2), nullptr, 16);
    Uint8 r = std::stoi(colorStr.substr(3, 2), nullptr, 16);
    Uint8 g = std::stoi(colorStr.substr(5, 2), nullptr, 16);
    Uint8 b = std::stoi(colorStr.substr(7, 2), nullptr, 16);

    return SDL_Color{r, g, b, a};
}

Config::~Config()
{
}
