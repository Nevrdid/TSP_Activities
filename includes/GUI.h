#pragma once

#include "DB.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// Constants for the screen size
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

using std::string;

class GUI
{
  private:
    SDL_Window*   window;
    SDL_Renderer* renderer;
    TTF_Font*     font;

    bool             is_running;
    size_t           selected_index;
    bool             in_game_detail;
    std::vector<Rom> roms_list;

    void render_text(
        const string& text, int x, int y, int size = 24, SDL_Color color = {255, 255, 255});
    void render_image(const string& image_path, int x, int y, int w, int h);

    void render_game_list();
    void render_game_detail();

    void sort_roms_by(const string& sort_by);

    void handle_inputs();

  public:
    GUI();
    ~GUI();
    void run(const std::string& rom_name = "");
};
