#pragma once
// Dessine une pastille verte à l'écran
void draw_green_dot(int x, int y, int radius = 8);

#if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#    error "No filesystem support"
#endif

#include "Config.h"
#include "DB.h"
#include "utils.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define LIST_OVERLAY APP_DIR "assets/list_overlay.png"
#define DETAILS_OVERLAY APP_DIR "assets/details_overlay.png"

#define FONT APP_DIR "assets/Lato-Medium.ttf"

#define FONT_ENORMOUS_SIZE 54
#define FONT_BIG_SIZE 42
#define FONT_MIDDLE_SIZE 30
#define FONT_TINY_SIZE 20
#define FONT_MINI_SIZE 16

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
    ZL,
    ZR,
    Select,
    Start,
    Menu,
    Quit,
};

enum MenuResult {
  Continue,
  ExitCurrent,
  ExitAll
};

typedef std::function<MenuResult()> MenuAction;

enum
{
    IMG_NONE = 0,
    IMG_FIT = 1,
    IMG_CENTER = 2
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

struct Text
{
    std::string str = "";
    int         size = 0;
    SDL_Color   color = {0, 0, 0, 0};

    bool operator==(const Text& other) const
    {
        return str == other.str && size == other.size && color.r == other.color.r &&
               color.g == other.color.g && color.b == other.color.b && color.a == other.color.a;
    };
};

struct CachedText
{
    SDL_Texture* texture = nullptr;
    int          width = 0;
    int          height = 0;
    Text         text;
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
    GUI();
    GUI(const GUI& copy);
    GUI& operator=(const GUI& copy);

    const Config& cfg;

    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Joystick* joystick;

    std::map<int, TTF_Font*> fonts;

    std::vector<CachedText>                    cached_text;
    std::unordered_map<std::string, CachedImg> image_cache;

    CachedText& getCachedText(const Text& text);

    bool         scroll_reset = false;
    SDL_Texture* background_texture = nullptr;
    // Track whether ra_hotkey existed when suspending a given game
    std::unordered_set<std::string> ra_hotkey_roms;
    // When true, remove /tmp/trimui_inputd/ra_hotkey while GUI is displayed
    bool keep_ra_hotkey_off = false;

    std::pair<MenuResult, size_t> _selector_core(const std::string& title,
        const std::vector<std::string>& labels, size_t max_width, bool center,
        size_t selected_index, const std::map<std::string, MenuAction>& actions);

  public:
    ~GUI();

    static GUI& getInstance()
    {
        static GUI instance;
        return instance;
    }

    void draw_green_dot(int x, int y, int radius = 8);
    void draw_circle(int x, int y, int radius, SDL_Color color, bool filled = true);
    // Draw a check mark centered at (x, y) with given size and color
    void draw_checkmark(int x, int y, int size, SDL_Color color);

    void clean();

    int init();

    int Width;
    int Height;

    InputAction map_input(const SDL_Event& e);
    void        clear();
    void        render();

    SDL_Texture* take_screenshot();

    Vec2 render_image(
        const std::string& image_path, int x, int y, int w = 0, int h = 0, int flags = IMG_CENTER);

    Vec2 render_text(const std::string& text, int x, int y, int font_size,
        SDL_Color color = {255, 255, 255, 255}, int width = 0, bool center = false);

    void render_multicolor_text(const vecColorString& colored_texts, int x, int y, int font_size);

    void render_scrollable_text(
        const std::string& text, int x, int y, int width, int font_size, SDL_Color color);
    void reset_scroll();

    void render_background(const std::string& system = "");
    void display_keybind(const std::string& btn, const std::string& text, int x);
    void display_keybind(
        const std::string& btn1, const std::string& btn2, const std::string& text, int x);
    TTF_Font* get_font(int size);

    void save_background_texture(SDL_Texture* required_texture = nullptr);
    void delete_background_texture();
    bool confirmation_popup(const std::string& message, int font_size);

    void message_popup(int duration, const std::vector<Text>& text_elements);
    // void message_popup(
    //     std::string title, int title_size, std::string message, int message_size, int duration);
    void infos_window(std::string title, int title_size,
        std::vector<std::pair<std::string, std::string>> content, int content_size, int x, int y,
        int width, int height);

    const std::string file_selector(fs::path location, bool hide_empties);

    const std::string string_selector(const std::string& title,
        const std::vector<std::string>& labels, size_t max_width, bool center);

    std::pair<MenuResult, size_t> menu_selector(const std::string& title,
        const std::vector<std::string>& labels, size_t max_width, bool center,
        size_t initial_selected_index, std::map<std::string, MenuAction> actions);

    MenuResult menu(const std::string& title, std::vector<std::pair<std::string, MenuAction>> menu_items);
};
