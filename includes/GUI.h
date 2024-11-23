#pragma once

#include "DB.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <set>
#include <string>
#include <vector>

// Constants for the screen size
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BACKGROUND APP_DIR "assets/Xmix_bg.png"
#define FONT APP_DIR "assets/Lato-Medium.ttf"
#define USE_KEYBOARD
#define X_0 50
#define Y_0 75
#define Y_LINE 50
#define LIST_LINES 10
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

class GUI
{
  private:
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Joystick* joystick;

    TTF_Font* font_tiny;
    TTF_Font* font_middle;
    TTF_Font* font_big;

    bool             is_running;
    size_t           selected_index;
    bool             in_game_detail;
    std::vector<Rom> roms_list;
    Sort             sort_by;

    std::vector<std::string> systems;
    size_t                   system_index = 0;
    std::vector<Rom>         filtered_roms_list;

    // filter completed flag: 0: all, 1: completed, 2: not completed
    int filter_completed = 0;

    void render_text(
        const string& text, int x, int y, TTF_Font* font, SDL_Color color = {255, 255, 255});
    void render_image(const string& image_path, int x, int y, int w, int h);

    void render_game_list();
    void render_game_detail();

    void handle_inputs();

    void set_completed();
    void filter();
    void sort();

  public:
    GUI();
    ~GUI();
    void run(const std::string& rom_name = "");
};
