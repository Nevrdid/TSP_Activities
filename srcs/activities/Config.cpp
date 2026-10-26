#include "Config.hpp"
#include "utils.hpp"

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
    // Native desktop builds do not have the TrimUI system configuration. Keep
    // a usable font fallback so an empty database can still be managed in the GUI.
    theme.font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

    nlohmann::json j;
    std::ifstream file("/mnt/UDISK/system.json");
    bool system_config_loaded = false;
    if (!file.is_open()) {
        std::cerr << "Info: /mnt/UDISK/system.json not found; using desktop defaults."
                  << std::endl;
    } else {
        try {
            file >> j;
            theme_path = j.value("theme", "");
            system_config_loaded = true;
        } catch (const std::exception& e) {
            std::cerr << "Warning: could not read /mnt/UDISK/system.json: " << e.what()
                      << "; using fallback settings." << std::endl;
            theme_path.clear();
        }
    }

    const std::string bundled_themes = utils::app_dir() + "Themes/";
    const std::string bundled_default = bundled_themes + "TRIMUI Blue/";

    // The desktop bundle carries its own themes. On TrimUI, preserve the
    // system-selected theme unless the legacy/default theme was requested.
    if (!system_config_loaded) {
        theme_path = bundled_default;
    } else if (theme_path == "../res/" || theme_path.empty()) {
        theme_path = "/mnt/SDCARD/Themes/TRIMUI YaHei/";
    }

    const bool may_select_theme =
        !system_config_loaded || theme_path == "/mnt/SDCARD/Themes/TRIMUI YaHei/" ||
        theme_path == bundled_default;
    const std::string theme_choice_file = utils::app_dir() + "data/theme.txt";
    std::ifstream theme_choice(theme_choice_file.c_str());
    if (may_select_theme && theme_choice.is_open()) {
        std::string choice;
        std::getline(theme_choice, choice);
        const std::set<std::string> supported_themes = {
            "TRIMUI YaHei", "TRIMUI Blue", "TRIMUI Energy",
            "TRIMUI HeyDW White", "TRIMUI Retro"
        };
        if (supported_themes.find(choice) != supported_themes.end()) {
            const std::string bundled_candidate = bundled_themes + choice + "/";
            const std::string handheld_candidate = "/mnt/SDCARD/Themes/" + choice + "/";
            std::error_code fs_error;
            if (fs::is_regular_file(fs::path(bundled_candidate) / "config.json", fs_error)) {
                theme_path = bundled_candidate;
            } else if (!fs_error &&
                system_config_loaded &&
                fs::is_regular_file(fs::path(handheld_candidate) / "config.json", fs_error)) {
                theme_path = handheld_candidate;
            } else if (!choice.empty()) {
                std::cerr << "Warning: selected theme '" << choice
                          << "' is not available in the app bundle or system theme folder."
                          << std::endl;
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
        theme.font = j.value("font", theme.font);
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
