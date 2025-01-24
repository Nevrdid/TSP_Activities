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

enum class InputAction
{
    None,
    Up,
    Down,
    Left,
    Right,
    A,
    B,
    X,
    Y,
    L1,
    R1,
    Select,
    Start,
    Menu,
    Quit,
};

static std::unordered_map<std::string, std::string> buttons_icons = {
    {"A", "tips-A.png"},
    {"B", "tips-B.png"},
    {"X", "button-tips-X.png"},
    {"Y", "button-tips-Y.png"},
    {"L1", "tips-L.png"},
    {"R1", "tips-R.png"},
    {"Select", "tips-SELECT.png"},
    {"Start", "button-tips-START.png"},
    {"Menu", "tips-MENU.png"},
};

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
    int          r = 0, g = 0, b = 0, a = 255;
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
    SDL_Texture* background_texture = nullptr;

  public:
    GUI(const Config& cfg);
    ~GUI();
    void clean();

    int init();

    InputAction map_input(const SDL_Event& e);
    void        clear_screen();
    void        render();

    void launch_external(const std::string& command);
    Vec2 render_image(const std::string& image_path, int x, int y, int w = 0, int h = 0,
        bool no_overflow = false, bool center = true);

    void render_text(const std::string& text, int x, int y, TTF_Font* font,
        SDL_Color color = {255, 255, 255, 255}, int width = 0, bool center = false);

    void render_multicolor_text(const vecColorString& colored_texts, int x, int y, TTF_Font* font);

    void render_scrollable_text(
        const std::string& text, int x, int y, int width, TTF_Font* font, SDL_Color color);
    void reset_scroll();

    void render_background(const std::string& system = "");
    void display_keybind(const std::string& btn, const std::string& text, int x, int y,
        TTF_Font* font, SDL_Color color);
    void display_keybind(const std::string& btn1, const std::string& btn2, const std::string& text,
        int x, int y, TTF_Font* font, SDL_Color color);

    void load_background_texture();
    void unload_background_texture();
    bool confirmation_popup(const std::string& message, TTF_Font* font);
};
