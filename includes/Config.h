#pragma once
#include "json.hpp"

#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

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

    size_t      width;
    size_t      height;

    SDL_Color   primary_color = {0, 255, 0, 255};
    SDL_Color   secondary_color = {255, 238, 180, 255};
    SDL_Color   background_color = {0, 0, 0, 0};
    std::string backgrounds_theme = "Default";
    std::string skins_theme = "CrossMix - OS";
    bool        theme_background = true;
    std::string default_background = "";

    std::string theme_path = "";
    Theme theme;
};
