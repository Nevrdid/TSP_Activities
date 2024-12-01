#pragma once

#include "Config.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define LIST_OVERLAY APP_DIR "assets/list_overlay.png"
#define DETAILS_OVERLAY APP_DIR "assets/details_overlay.png"

#define FONT APP_DIR "assets/Lato-Medium.ttf"

#define FONT_BIG_SIZE 72
#define FONT_MIDDLE_SIZE 54
#define FONT_TINY_SIZE 32
#define FONT_MINI_SIZE 24

#define LIST_LINES 6

struct Vec2
{
    int x = 0;
    int y = 0;
};

struct CachedText
{
    SDL_Texture* texture = nullptr;
    int          width = 0;
    int          height = 0;
    std::string  text = "";
    int          r = 0, g = 0, b = 0;
};

struct CachedImg
{
    SDL_Texture* texture = nullptr;
    int          width = 0;
    int          height = 0;
};

typedef std::vector<std::pair<std::string, SDL_Color>> vecColorString;

class GUI
{
  private:
    const Config& cfg;

    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Joystick* joystick;

    std::vector<CachedText>                    cached_text;
    std::unordered_map<std::string, CachedImg> image_cache;

    CachedText& getCachedText(const std::string& text, TTF_Font* font, SDL_Color color);

    bool scroll_reset = false;

  public:
    GUI(const Config& cfg);
    ~GUI();

    int  init();
    void handle_inputs();
    void clear_screen();
    void render();

    void launch_external(const std::string& command);
    Vec2 render_image(const std::string& image_path, int x, int y, int w = 0, int h = 0,
        bool no_overflow = false);

    void render_text(const std::string& text, int x, int y, TTF_Font* font,
        SDL_Color color = {255, 255, 255, 255}, int width = 0, bool center = false);

    void render_multicolor_text(const vecColorString& colored_texts, int x, int y, TTF_Font* font);

    void render_scrollable_text(
        const std::string& text, int x, int y, int width, TTF_Font* font, SDL_Color color);
    void reset_scroll();

    void render_background(const std::string& system = "");
};
