#include "GUI.h"

#include <regex>
GUI::GUI()
    : window(nullptr)
    , renderer(nullptr)
    , is_running(true)
    , selected_index(0)
    , in_game_detail(false)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        is_running = false;
        return;
    }

    window = SDL_CreateWindow("Game Timer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        is_running = false;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        is_running = false;
        return;
    }

    if (TTF_Init()) {
        std::cerr << "Failed to init ttf: " << TTF_GetError() << std::endl;
        return;
    }

    font = TTF_OpenFont("assets/arial.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }
}

GUI::~GUI()
{
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GUI::render_image(const std::string& image_path, int x, int y, int w, int h)
{
    SDL_Surface* surface = IMG_Load(image_path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from surface: " << image_path << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dest_rect = {x, y, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void GUI::render_text(const std::string& text, int x, int y, int size, SDL_Color color)
{

    TTF_SetFontSize(font, size);
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect dest_rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void GUI::render_game_list()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear screen with black
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    size_t roms_amt = roms_list.size();
    size_t first = selected_index < 5              ? 0
                : selected_index < roms_amt - 5 ? selected_index
                                                : roms_amt - 10;

    for (size_t i = first; i < first + 10; ++i) {
        SDL_Color color = (i == selected_index) ? yellow : white;
        render_text(roms_list[i].name + " - " + std::to_string(roms_list[i].time) + "s", 50,
            50 + i * 30, 24, color);
    }

    SDL_RenderPresent(renderer);
}

void GUI::render_game_detail()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear screen with black
    SDL_RenderClear(renderer);

    const Rom& rom = roms_list[selected_index];
    SDL_Color  white = {255, 255, 255, 255};

    // Header
    render_text(rom.name, 50, 20, 24, white);

    // rom image
    std::regex pattern(R"(\/Roms\/([^\/]+))"); // Matches "/Roms/<subfolder>"
    string     rom_image = std::regex_replace(rom.file, pattern, R"(/Imgs/$1)") + rom.name + ".png";

    render_image("/mnt/SDCARD/Imgs/", 50, 100, 200, 200);

    // rom stats
    render_text("Total Time: " + std::to_string(rom.time) + "s", 300, 100, 20, white);
    render_text("Avg Time: " + std::to_string(rom.count ? rom.time / rom.count : 0) + "s", 300, 150,
        20, white);
    render_text("Play Count: " + std::to_string(rom.count), 300, 200, 20, white);
    render_text("Last Played: " + rom.last, 300, 250, 20, white);

    SDL_RenderPresent(renderer);
}

void GUI::handle_inputs()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            is_running = false;
        } else if (e.type == SDL_JOYHATMOTION) {
            switch (e.jhat.value) {
            case SDL_HAT_UP: // D-Pad Up
                if (!in_game_detail && selected_index > 0) selected_index--;
                break;
            case SDL_HAT_DOWN: // D-Pad Down
                if (!in_game_detail && selected_index < roms_list.size() - 1) selected_index++;
                break;
            case SDL_HAT_LEFT: // D-Pad Left
                if (in_game_detail && selected_index < roms_list.size() - 1) selected_index++;
                break;
            case SDL_HAT_RIGHT: // D-Pad Right
                if (!in_game_detail && selected_index > 0) selected_index--;
                break;
            }
        } else if (e.type == SDL_JOYBUTTONDOWN) {
            switch (e.key.keysym.sym) {
            case 0: // B (b0)
                if (in_game_detail) in_game_detail = false;
                else is_running = false;
                break;
            case 1: // A (b1)
                in_game_detail = true;
                break;
            case 2: // X (b2)
                break;
            case 3: // Y (b3)
                break;
            case 4: // L1 (b4)
                break;
            case 5: // R1 (b5)
                break;
            case 6: // Back button (b6)
                break;
            case 7: // Start button (b7)
                break;
            case 8: // Guide button (Home) (b8)
                is_running = false;
                break;
            case 9: // Left Stick Button (L3) (b9)
                break;
            case 10: // Right Stick Button (R3) (b10)
                break;
            }
        }
    }
}

void GUI::run(const std::string& rom_name)
{

    std::cout << "Entering GUI." << std::endl;
    DB db;
    if (rom_name == "") {
        roms_list = db.load_all();
    } else {
        in_game_detail = true;
        roms_list.push_back(db.load(rom_name));
    }

    std::cout << "Starting gui." << std::endl;
    while (is_running) {
        handle_inputs();
        if (in_game_detail) {
            render_game_detail();
        } else {
            render_game_list();
        }
        SDL_Delay(16); // ~60 FPS
    }
}