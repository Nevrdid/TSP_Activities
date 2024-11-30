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
                    if (key == "device") {
                        if (value == "tsp") {
                            width = 1280;
                            height = 720;
                            list_mid = 430;
                            list_dy = 8;

                            list_y0 = 125;
                            list_y1 = 680;
                            list_x0 = 10;
                            list_x1 = 600;
                            list_x2 = 830;

                            details_img_size = 550;
                            details_y0 = 220;
                            details_y1 = 650;
                            details_x0 = 10;
                            details_x1 = 600;
                            details_x2 = 830;
                        } else if (value == "brick") {
                            width = 1024;
                            height = 768;

                            list_mid = 512;
                            list_dy = 8;
                            list_y0 = 140;
                            list_y1 = 687;
                            list_x0 = 20;
                            list_x1 = 490;
                            list_x2 = 670;

                            details_img_size = 450;
                            details_y0 = 220;
                            details_y1 = 687;
                            details_x0 = 8;
                            details_x1 = 490;
                            details_x2 = 670;
                        }
                    } else if (key == "backgrounds_theme") {
                        backgrounds_theme = value;
                    } else if (key == "skins_theme") {
                        skins_theme = value;
                    } else if (key == "primary_color") {
                        primary_color = colors.at(value);
                    } else if (key == "secondary_color") {
                        secondary_color = colors.at(value);
                    }
                }
            }
        }
    }
    theme_path = "/mnt/SDCARD/Themes/" + skins_theme + "/";
    std::string theme_config = theme_path + "/config.json";
    if (std::filesystem::exists(theme_config))
        load_theme(theme_config);
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
