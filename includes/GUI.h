#pragma once

#include "DB.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Constants for the screen size
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BACKGROUND APP_DIR "assets/Xmix_bg.png"
#define FONT APP_DIR "assets/Lato-Medium.ttf"
#define FONT_BIG_SIZE 72
#define FONT_MIDDLE_SIZE 54
#define FONT_TINY_SIZE 36
#define FONT_MINI_SIZE 24
#define LIST_LINES 5
#define X_0 50
#define Y_0 55
#define Y_LINE 540 / LIST_LINES
#define FALLBACK_PICTURE APP_DIR "assets/placeholder.png"

enum Sort
{
    e_name,
    e_time,
    e_count,
    e_last
};

static const std::string sort_names[] = {"Name", "Time", "Count", "Last"};

static const std::string completed_names[] = {"All", "Completed", "Not completed"};

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

    // FONTS
    TTF_Font* font_big;
    TTF_Font* font_middle;
    TTF_Font* font_tiny;
    TTF_Font* font_mini;

    // Datas

    // GUI
    bool   is_running = true;
    size_t selected_index = 0;
    bool   in_game_detail = false;

    std::vector<Rom> roms_list;

    // SORT && FILTERS
    std::vector<Rom> filtered_roms_list;
    size_t           list_size = 0;

    Sort                     sort_by = e_time;
    std::vector<std::string> systems;
    size_t                   system_index = 0;
    int                      filter_completed = 0;
    // filter completed flag: 0: all, 1: completed, 2: not completed

    size_t scroll_finished = true;

    void handle_inputs();

    void render_game_list();
    void filter();
    void sort();

    void render_game_detail();
    void switch_completed();

    void        render_image(const string& image_path, int x, int y, int w, int h);
    CachedText& getCachedText(const std::string& text, TTF_Font* font, SDL_Color color);
    void        render_text(const string& text, int x, int y, TTF_Font* font,
               SDL_Color color = {255, 255, 255}, int width = 0);
    void render_multicolor_text(const vecColorString& colored_texts, int x, int y, TTF_Font* font);
    void render_scrollable_text(
        const std::string& text, int x, int y, int width, TTF_Font* font, SDL_Color color);

  public:
    GUI();
    ~GUI();
    void run(const std::string& rom_name = "");
};
