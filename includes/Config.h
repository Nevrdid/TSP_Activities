#pragma once

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "No filesystem support"
#endif

#include <SDL.h>
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#define CONFIG_FILE APP_DIR "data/config.ini"

using json = nlohmann::json;

static const std::unordered_map<std::string, SDL_Color> colors = {
    {"black", SDL_Color{0, 0, 0, 255}},
    {"white", SDL_Color{255, 255, 255, 255}},
    {"red", SDL_Color{255, 0, 0, 255}},
    {"green", SDL_Color{0, 255, 0, 255}},
    {"blue", SDL_Color{0, 0, 255, 255}},
    {"yellow", SDL_Color{255, 255, 0, 255}},
    {"cyan", SDL_Color{0, 255, 255, 255}},
    {"magenta", SDL_Color{255, 0, 255, 255}},
    {"orange", SDL_Color{255, 165, 0, 255}},
    {"purple", SDL_Color{128, 0, 128, 255}},
    {"pink", SDL_Color{255, 192, 203, 255}},
    {"brown", SDL_Color{165, 42, 42, 255}},
    {"grey", SDL_Color{128, 128, 128, 255}},
    {"lightgrey", SDL_Color{211, 211, 211, 255}},
    {"darkgrey", SDL_Color{169, 169, 169, 255}},
    {"lightblue", SDL_Color{173, 216, 230, 255}},
    {"lightgreen", SDL_Color{144, 238, 144, 255}},
    {"lightyellow", SDL_Color{255, 255, 224, 255}},
    {"lightcyan", SDL_Color{224, 255, 255, 255}},
    {"lightmagenta", SDL_Color{255, 224, 255, 255}},
    {"lightorange", SDL_Color{255, 218, 185, 255}},
    {"lightpurple", SDL_Color{230, 230, 250, 255}},
    {"lightpink", SDL_Color{255, 182, 193, 255}},
    {"lightbrown", SDL_Color{210, 105, 30, 255}},
};

struct Theme
{
    std::string                                name;
    std::string                                font;
    std::string                                fontAscii;
    std::unordered_map<std::string, int>       fontSize;
    std::unordered_map<std::string, SDL_Color> fontColor;
};

class Config
{
  private:
    bool      load_theme(const std::string& filePath);
    SDL_Color parseColor(const std::string& colorStr) const;

  public:
    Config();
    ~Config();

    SDL_Color   selected_color = {0, 255, 0, 255};
    SDL_Color   unselect_color = {255, 238, 180, 255};
    SDL_Color   title_color = {0, 255, 0, 255};
    SDL_Color   info_color = {255, 238, 180, 255};
    SDL_Color   background_color = {0, 0, 0, 0};
    std::string backgrounds_theme = "Default";
    std::string skins_theme = "TRIMUI Blue";
    bool        theme_background = true;
    std::string default_background = "";

    std::string theme_path = "";
    Theme       theme;
};
