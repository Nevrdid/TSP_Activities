#include "GUI.h"

#include <regex>
GUI::GUI()
    : window(nullptr)
    , renderer(nullptr)
    , is_running(true)
    , selected_index(0)
    , in_game_detail(false)
    , sort_by(e_time)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        is_running = false;
        return;
    }

    joystick = SDL_JoystickOpen(0);
    if (joystick == NULL) {
        std::cerr << "Couldn't open joystick 0: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "Joystick 0 opened successfully." << std::endl;
    }
    SDL_JoystickEventState(SDL_ENABLE);

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

    font_tiny = TTF_OpenFont(FONT, 20);
    font_middle = TTF_OpenFont(FONT, 30);
    font_big = TTF_OpenFont(FONT, 42);

    if (!font_tiny || !font_middle || !font_big) {
        std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
        is_running = false;
        return;
    }
}

GUI::~GUI()
{
    TTF_CloseFont(font_tiny);
    TTF_CloseFont(font_middle);
    TTF_CloseFont(font_big);
    SDL_JoystickClose(joystick);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GUI::set_completed()
{
    Rom& rom = filtered_roms_list[selected_index];
    DB   db;
    for (auto& r : roms_list) {
        if (r.file == rom.file) {
            r = rom = db.save(rom.file, 0, rom.completed ? 0 : 1);
            break;
        }
    }
}

void GUI::filter()
{
    const std::string& current_system = systems[system_index];

    filtered_roms_list.clear();
    for (const auto& rom : roms_list) {
        if (rom.system == current_system || system_index == 0) {
            if (filter_completed == 0) {
                filtered_roms_list.push_back(rom);
            } else if (filter_completed == 1 && rom.completed == 1) {
                filtered_roms_list.push_back(rom);
            } else if (filter_completed == 2 && rom.completed == 0) {
                filtered_roms_list.push_back(rom);
            }
        }
    }
    list_size = filtered_roms_list.size();
    sort();

    if (selected_index >= filtered_roms_list.size()) {
        selected_index = filtered_roms_list.empty() ? 0 : filtered_roms_list.size() - 1;
    }
}

void GUI::sort()
{
    switch (sort_by) {
    case e_name:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.name < b.name; });
        break;
    case e_time:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.total_time > b.total_time; });
        break;
    case e_count:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.count > b.count; });
        break;
    case e_last:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.last > b.last; });
        break;
    }
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

void GUI::render_text(const std::string& text, int x, int y, TTF_Font* font, SDL_Color color)
{

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

static SDL_Color white = {255, 255, 255, 255};
static SDL_Color yellow = {255, 238, 180, 255};
static SDL_Color green = {175, 248, 200, 255};

void GUI::render_game_list()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_image(BACKGROUND, 0, 0, 1280, 720);

    render_text("- " + completed_names[filter_completed] + " " + systems[system_index] + " Games -",
        X_0 + 50, Y_0, font_big, yellow);

    size_t first = selected_index < LIST_LINES / 2               ? 0
                   : selected_index < list_size - LIST_LINES / 2 ? selected_index - LIST_LINES / 2
                                                                 : list_size - LIST_LINES;
    size_t last = first + LIST_LINES < filtered_roms_list.size() ? first + LIST_LINES
                                                                 : filtered_roms_list.size();

    int y = Y_0 + 75;
    for (size_t j = first; j < last; ++j) {
        SDL_Color color = (j == selected_index) ? green : white;
        render_text(filtered_roms_list[j].name + " - " + filtered_roms_list[j].total_time, X_0, y,
            font_middle, color);
        y += Y_LINE;
    }

    render_text("A: Select  B: Quit  X: Filter (Un)Completed  Y: Filter oldest  L/R: Change system "
                "Select: Sort by (" +
                    sort_names[sort_by] + ")",
        X_0, SCREEN_HEIGHT - 35, font_tiny, white);

    SDL_RenderPresent(renderer);
}

void GUI::render_game_detail()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_image(BACKGROUND, 0, 0, 1280, 720);

    const Rom& rom = filtered_roms_list[selected_index];

    // Game name
    render_text("- " + rom.name + " -", X_0 + 50, Y_0, font_big, yellow);

    // Left side: Rom image
    render_image(rom.image, X_0, Y_0 + 100, 450, 450);

    // Right side: Game details
    render_text("Total Time:", X_0 + 500, Y_0 + 100, font_middle, green);
    render_text(rom.total_time, X_0 + 750, Y_0 + 100, font_middle, white);

    render_text("Average Time:", X_0 + 500, Y_0 + 150, font_middle, green);
    render_text(rom.average_time, X_0 + 750, Y_0 + 150, font_middle, white);

    render_text("Play count:", X_0 + 500, Y_0 + 200, font_middle, green);
    render_text(std::to_string(rom.count), X_0 + 750, Y_0 + 200, font_middle, white);

    render_text("Last played:", X_0 + 500, Y_0 + 250, font_middle, green);
    render_text(rom.last, X_0 + 750, Y_0 + 250, font_middle, white);

    render_text("System:", X_0 + 500, Y_0 + 300, font_middle, green);
    render_text(rom.system, X_0 + 750, Y_0 + 300, font_middle, white);

    render_text("Completed:", X_0 + 500, Y_0 + 350, font_middle, green);
    render_text(rom.completed ? "Yes" : "No", X_0 + 750, Y_0 + 350, font_middle, white);

    // Bottom file path.

    render_text("File:", X_0, Y_0 + 560, font_tiny, green);
    render_text(rom.file, X_0 + 50, Y_0 + 560, font_tiny, white);

    // Footer keybinds.
    render_text(
        "A: Launch  B: Return X: Set (Un)Completed", X_0, SCREEN_HEIGHT - 35, font_tiny, white);

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
                if (!in_game_detail && selected_index < filtered_roms_list.size() - 1)
                    selected_index++;
                break;
            case SDL_HAT_LEFT: // D-Pad Left
                if (!in_game_detail) selected_index = selected_index > 9 ? selected_index - 10 : 0;
                break;
            case SDL_HAT_RIGHT: // D-Pad Right
                if (!in_game_detail)
                    selected_index = list_size > 10 && selected_index < list_size - 10
                                         ? selected_index + 10
                                         : list_size - 1;
                break;
            }
        } else if (e.type == SDL_JOYBUTTONDOWN) {
            switch (e.jbutton.button) {
            case 0: // B (b0)
                if (in_game_detail) {
                    in_game_detail = false;
                    filter();
                } else {
                    is_running = false;
                }
                break;
            case 1: // A (b1)
                in_game_detail = true;
                break;
            case 2: // Y (b2)
                break;
            case 3: // X (b3)
                if (in_game_detail) {
                    set_completed();
                } else {
                    filter_completed = (filter_completed + 1) % 3;
                    filter();
                }
                break;
            case 4: // L1 (b4)
                if (!systems.empty()) {
                    system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
                    filter();
                }
                break;
            case 5: // R1 (b5)
                if (!systems.empty()) {
                    system_index = (system_index + 1) % systems.size();
                    filter();
                }
                break;
            case 6: // Back button (b6)
                if (!in_game_detail) {
                    if (sort_by == e_last) {
                        sort_by = e_name;
                    } else {
                        sort_by = static_cast<Sort>(sort_by + 1);
                    }
                    sort();
                }
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
#if defined(USE_KEYBOARD)
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_UP:
                if (!in_game_detail && selected_index > 0) selected_index--;
                break;
            case SDLK_DOWN:
                if (!in_game_detail && selected_index < filtered_roms_list.size() - 1)
                    selected_index++;
                break;
            case SDLK_LEFT:
                if (!in_game_detail) selected_index = selected_index > 10 ? selected_index - 10 : 0;
                break;
            case SDLK_RIGHT:
                if (!in_game_detail)
                    selected_index = list_size > 10 && selected_index < list_size - 10
                                         ? selected_index + 10
                                         : list_size - 1;
                break;
            case SDLK_s:
                if (!in_game_detail) {
                    if (sort_by == e_last) {
                        sort_by = e_name;
                    } else {
                        sort_by = static_cast<Sort>(sort_by + 1);
                    }
                    sort();
                }
                break;
            case SDLK_c:
                if (in_game_detail) {
                    set_completed();
                } else {
                    filter_completed = (filter_completed + 1) % 3;
                    filter();
                }
                break;
            case SDLK_g:
                if (!systems.empty()) {
                    system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
                    filter();
                }
                break;
            case SDLK_h:
                if (!systems.empty()) {
                    system_index = (system_index + 1) % systems.size();
                    filter();
                }
                break;
            case SDLK_RETURN:
                in_game_detail = true;
                break;
            case SDLK_ESCAPE:
                if (in_game_detail) {
                    in_game_detail = false;
                    filter();
                } else {
                    is_running = false;
                }
                break;
            }
        }
#endif
    }
}

void GUI::run(const std::string& rom_name)
{
    std::cout << "ActivitiesApp: Starting GUI." << std::endl;
    DB db;
    if (rom_name == "") {
        roms_list = db.load_all();
        if (roms_list.empty()) {
            std::cerr << "ActivitiesApp: The database is empty. Leaving..." << std::endl;
            return;
        }

        std::set<std::string> unique_systems;
        for (const auto& rom : roms_list) {
            unique_systems.insert(rom.system);
        }
        systems.push_back("");
        systems.insert(systems.end(), unique_systems.begin(), unique_systems.end());

        filter();
    } else {
        in_game_detail = true;
        roms_list.push_back(db.load(rom_name));
        filtered_roms_list = roms_list;
    }

    std::cout << "ActivitiesApp: Roms datas loaded." << std::endl;
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
