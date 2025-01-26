#include "Config.h"

Config::Config()
{
    std::ifstream file(std::string(APP_DIR) + "data/config.ini");

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string        key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    if (key == "backgrounds_theme") {
                        backgrounds_theme = value;
                    } else if (key == "skins_theme") {
                        skins_theme = value;
                    }
                }
            }
        }
    }
    theme_path = "/mnt/SDCARD/Themes/" + skins_theme + "/";
    std::string theme_config = theme_path + "/config.json";
    if (std::filesystem::exists(theme_config))
        load_theme(theme_config);
    selected_color = theme.fontColor["content_color4"];
    unselect_color = theme.fontColor["content_color1"];
    title_color = theme.fontColor["nav_color1"];
    info_color = theme.fontColor["stat_color1"];
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

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
        return false;
    }
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
