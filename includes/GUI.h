#pragma once

#include "DB.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

// Constants for the screen size
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define CFG_FILE APP_DIR "activities.cfg"

#define LIST_OVERLAY APP_DIR "assets/list_overlay.png"
#define DETAILS_OVERLAY APP_DIR "assets/details_overlay.png"

#define FONT APP_DIR "assets/Lato-Medium.ttf"
#define FONT_BIG_SIZE 72
#define FONT_MIDDLE_SIZE 54
#define FONT_TINY_SIZE 32
#define FONT_MINI_SIZE 24

#define LIST_LINES 6
#define X_0 10
#define Y_0 0
#define Y_LINE 590 / LIST_LINES

enum Sort
{
    e_name,
    e_time,
    e_count,
    e_last
};

static const std::string sort_names[] = {"Name", "Time", "Count", "Last"};

static const std::string completed_names[] = {"All", "Completed", "Not completed"};

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

using std::string;

struct CachedText
{
    SDL_Texture* texture = nullptr;
    int          width = 0;

    std::string text = "";
    int         r = 0;
    int         g = 0;
    int         b = 0;
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
    // SDL
    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Joystick* joystick;

    // CACHE
    std::vector<CachedText>                    cached_text;
    std::unordered_map<std::string, CachedImg> image_cache;
    SDL_Surface*                               scroll_surface = nullptr;
    SDL_Texture*                               scroll_texture = nullptr;

    CachedText& getCachedText(const std::string& text, TTF_Font* font, SDL_Color color);

    // FONTS
    TTF_Font* font_big;
    TTF_Font* font_middle;
    TTF_Font* font_tiny;
    TTF_Font* font_mini;

    std::string theme = "Default";
    bool        theme_background = true;
    std::string default_background = "black";
    SDL_Color   primary_color = {0, 255, 0, 255};
    SDL_Color   secondary_color = {255, 238, 180, 255};

    // Datas

    // GUI
    bool is_running = true;
    bool in_game_detail = false;
    bool no_list = false;

    std::vector<Rom> roms_list;
    void             load_config_file();
    void             handle_inputs();

    std::vector<Rom> filtered_roms_list;
    size_t           selected_index = 0;
    size_t           list_size = 0;
    void             render_game_list();

    Sort                     sort_by = e_time;
    std::vector<std::string> systems;
    size_t                   system_index = 0;
    int                      filter_completed = 0;
    // filter completed flag: 0: all, 1: completed, 2: not completed
    void sort();
    void filter();

    void render_background(const std::string& overlay);

    void render_game_detail();

    void switch_completed();
    void launch_external(const std::string& command);

    // Rendering
    void render_image(
        const string& image_path, int x, int y, int w = 0, int h = 0, bool no_overflow = false);

    void render_text(const string& text, int x, int y, TTF_Font* font,
        SDL_Color color = {255, 255, 255}, int width = 0);

    void render_multicolor_text(const vecColorString& colored_texts, int x, int y, TTF_Font* font);

    size_t scroll_finished = true;
    void   render_scrollable_text(
          const std::string& text, int x, int y, int width, TTF_Font* font, SDL_Color color);

  public:
    GUI();
    ~GUI();
    void init(const std::string& rom_name = "");
    void run();
};
