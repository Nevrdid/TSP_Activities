#include "GUI.h"

#include <regex>
GUI::GUI()
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

    font_big = TTF_OpenFont(FONT, FONT_BIG_SIZE);
    font_middle = TTF_OpenFont(FONT, FONT_MIDDLE_SIZE);
    font_tiny = TTF_OpenFont(FONT, FONT_TINY_SIZE);
    font_mini = TTF_OpenFont(FONT, FONT_MINI_SIZE);

    if (!font_tiny || !font_middle || !font_big || !font_mini) {
        std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
        is_running = false;
        return;
    }
}

GUI::~GUI()
{
    // Clean cache
    for (auto& texture : image_cache) {
        if (texture.second.texture) {
            SDL_DestroyTexture(texture.second.texture);
        }
    }
    image_cache.clear();

    for (auto& entry : cached_text) {
        if (entry.texture) {
            SDL_DestroyTexture(entry.texture);
        }
    }
    cached_text.clear();

    if (scroll_surface) SDL_FreeSurface(scroll_surface);
    if (scroll_texture) SDL_DestroyTexture(scroll_texture);

    TTF_CloseFont(font_tiny);
    TTF_CloseFont(font_middle);
    TTF_CloseFont(font_big);
    SDL_JoystickClose(joystick);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GUI::load_config_file()
{
    const std::string& config_file = std::string(APP_DIR) + "activities.cfg";
    std::ifstream      file(config_file);

    // read it and save theme and theme_background values ( first is a string, second is a bool)
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string        key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    if (key == "theme_name") {
                        theme = value;
                    } else if (key == "theme_background") {
                        theme_background = value == "true";
                    } else if (key == "default_background") {
                        default_background = value;
                    }
                }
            }
        }
    }
}

void GUI::switch_completed()
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
    scroll_finished = true;
}

void GUI::launch_external(const std::string& command)
{
    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_DISABLE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_DISABLE);

    std::cout << "ActivitiesApp: Launching " << command << std::endl;

    system(command.c_str());

    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);
    while (SDL_WaitEvent(NULL) < 0) {
    }
    SDL_FlushEvents(SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP);
}

void GUI::render_image(const std::string& image_path, int x, int y, int w, int h, bool no_overflow)
{
    if (image_path.empty()) return;
    if (image_cache.find(image_path) == image_cache.end()) {
        SDL_Surface* surface = IMG_Load(image_path.c_str());
        image_cache[image_path] = {
            SDL_CreateTextureFromSurface(renderer, surface), surface->w, surface->h};
        SDL_FreeSurface(surface);
    }
    CachedImg& cached_texture = image_cache[image_path];

    int width = w;
    int height = h;
    if (h == 0 && w) {
        height = w * cached_texture.height / cached_texture.width;
        if (no_overflow && height > width) {
            height = width;
            width = height * cached_texture.width / cached_texture.height;
        }
    } else if (w == 0 && h) {
        width = h * cached_texture.width / cached_texture.height;
        if (no_overflow && width > height) {
            width = height;
            height = width * cached_texture.height / cached_texture.width;
        }
    } else if (w == 0 && h == 0) {
        width = cached_texture.width;
        height = cached_texture.height;
    }

    int      x0 = x - width / 2;
    int      y0 = y - height / 2;
    SDL_Rect dest_rect = {x0, y0, width, height};
    SDL_RenderCopy(renderer, cached_texture.texture, nullptr, &dest_rect);
}

void GUI::render_multicolor_text(
    const std::vector<std::pair<std::string, SDL_Color>>& colored_texts, int x, int y,
    TTF_Font* font)
{
    int current_x = x;
    for (const auto& text_pair : colored_texts) {
        const std::string& text = text_pair.first;
        SDL_Color          color = text_pair.second;

        CachedText& cached = getCachedText(text, font, color);

        if (!cached.texture) {
            std::cerr << "Failed to retrieve or cache text texture." << std::endl;
            return;
        }

        SDL_Rect dest_rect = {current_x, y, cached.width, TTF_FontHeight(font)};

        SDL_RenderCopy(renderer, cached.texture, nullptr, &dest_rect);

        current_x += cached.width;
    }
}

CachedText& GUI::getCachedText(const std::string& text, TTF_Font* font, SDL_Color color)
{
    for (auto& cached : cached_text) {
        if (cached.text == text && cached.r == color.r && cached.g == color.g &&
            cached.b == color.b) {
            return cached;
        }
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }

    CachedText cached = {texture, surface->w, text, color.r, color.g, color.b};
    cached_text.push_back(cached);

    SDL_FreeSurface(surface);
    return cached_text.back();
}

void GUI::render_text(
    const std::string& text, int x, int y, TTF_Font* font, SDL_Color color, int width)
{
    CachedText& cached = getCachedText(text, font, color);

    if (!cached.texture) {
        std::cerr << "Failed to retrieve or cache text texture." << std::endl;
        return;
    }

    if (width && cached.width > width) {
        SDL_Rect render_rect = {x, y, width, TTF_FontHeight(font)};
        SDL_Rect clip_rect = {0, 0, width, TTF_FontHeight(font)};
        SDL_RenderCopy(renderer, cached.texture, &clip_rect, &render_rect);
    } else {
        SDL_Rect dest_rect = {x, y, cached.width, TTF_FontHeight(font)};
        SDL_RenderCopy(renderer, cached.texture, nullptr, &dest_rect);
    }
}

void GUI::render_scrollable_text(
    const std::string& text, int x, int y, int width, TTF_Font* font, SDL_Color color)
{

    static int    offset = 0;
    static Uint32 last_update = 0;

    if (scroll_finished) {
        if (scroll_surface) SDL_FreeSurface(scroll_surface);
        if (scroll_texture) SDL_DestroyTexture(scroll_texture);
        scroll_surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!scroll_surface) {
            std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
            TTF_CloseFont(font);
            return;
        }
        scroll_texture = SDL_CreateTextureFromSurface(renderer, scroll_surface);

        scroll_finished = false;
        offset = 0;
    }

    if (scroll_surface->w <= width) {
        render_text(text, x, y, font, color, width);
        return;
    }

    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_update > (offset > 0 ? 16 : 2000)) { // Adjust delay
        last_update = current_time;

        if (offset >= 0) {
            offset += 3; // Scrolling speed

            if (offset > scroll_surface->w - width) {
                if (!scroll_finished) {
                    scroll_finished = true;
                }
            }
        }
    }

    SDL_Rect clip_rect = {offset, 0, width, TTF_FontHeight(font)};
    SDL_Rect render_rect = {x, y, width, TTF_FontHeight(font)};

    SDL_RenderCopy(renderer, scroll_texture, &clip_rect, &render_rect);
}
static SDL_Color white = {255, 255, 255, 255};
static SDL_Color yellow = {255, 238, 180, 255};
static SDL_Color green = {175, 248, 200, 255};

void GUI::render_game_list()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    std::string background = std::string(APP_DIR) + "assets/" + default_background;
    if (theme_background) {
        std::string system_bg =
            "/mnt/SDCARD/Backgrounds/" + theme + "/" + systems[system_index] + ".png";
        if (std::filesystem::exists(system_bg)) {
            render_image(
                system_bg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
        } else {
            render_image(
                background, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    } else {
        render_image(background, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    render_image(LIST_BG, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);

    render_text(completed_names[filter_completed] + " " + systems[system_index] + " Games",
        X_0 + 75, Y_0, font_big, yellow);

    size_t first;
    size_t last;
    if (list_size <= LIST_LINES) {
        first = 0;
        last = list_size;
    } else {
        first = selected_index < LIST_LINES / 2               ? 0
                : selected_index < list_size - LIST_LINES / 2 ? selected_index - LIST_LINES / 2
                                                              : list_size - LIST_LINES;
        last = first + LIST_LINES < filtered_roms_list.size() ? first + LIST_LINES
                                                              : filtered_roms_list.size();
    }

    std::vector<std::pair<std::string, SDL_Color>> multi_color_line;
    int                                            y = Y_0 + 75;
    for (size_t j = first; j < last; ++j) {
        const Rom& rom = filtered_roms_list[j];
        SDL_Color  color = (j == selected_index) ? green : white;
        if (j == selected_index) {
            render_scrollable_text(rom.name, X_0, y, 840, font_middle, color);
        } else {
            render_text(rom.name, X_0, y, font_middle, color, 840);
        }

        y += Y_LINE / 2;

        multi_color_line = {{"Time: ", yellow}, {rom.total_time, color}, {"  Count: ", yellow},
            {std::to_string(rom.count), color}, {"  Last: ", yellow}, {rom.last, color}};
        render_multicolor_text(multi_color_line, X_0, y, font_tiny);

        y += Y_LINE / 2;
    }
    render_image(filtered_roms_list[selected_index].image, X_0 + 1058, 370, 390, 0);

    multi_color_line = {{"A: ", yellow}, {"Select", white}, {"  B: ", yellow}, {"Quit", white},
        {"  L/R: ", yellow}, {"Change system", white}, {"  Select: ", yellow},
        {"Filter (Un)Completed", white}, {"  Start: ", yellow}, {" Sort by (", white},
        {sort_names[sort_by], green}, {")", white}};
    render_multicolor_text(multi_color_line, X_0, SCREEN_HEIGHT - 35, font_mini);

    SDL_RenderPresent(renderer);
}

void GUI::render_game_detail()
{
    std::vector<std::pair<std::string, SDL_Color>> multi_color_line;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    std::string background = std::string(APP_DIR) + "assets/" + default_background;
    if (theme_background) {
        std::string system_bg =
            "/mnt/SDCARD/Backgrounds/" + theme + "/" + systems[system_index] + ".png";
        if (std::filesystem::exists(system_bg)) {
            render_image(
                system_bg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
        } else {
            render_image(
                background, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    } else {
        render_image(background, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    render_image(DETAILS_BG, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT);

    const Rom& rom = filtered_roms_list[selected_index];

    int y = Y_0 + 50;
    int x0 = X_0 + 25;
    int x1 = x0 + 600;
    int x2 = x1 + 230;

    // Game name
    render_scrollable_text(rom.name, 80, 0, 1120, font_middle, yellow);

    // Left side: Rom image
    render_image(rom.image, 300, 350, 0, 550, true);

    // Right side: Game details
    render_text("Total Time:", x1, y + 100, font_tiny, green);
    render_text(rom.total_time, x2, y + 100, font_tiny, white);

    render_text("Average Time:", x1, y + 150, font_tiny, green);
    render_text(rom.average_time, x2, y + 150, font_tiny, white);

    render_text("Play count:", x1, y + 200, font_tiny, green);
    render_text(std::to_string(rom.count), x2, y + 200, font_tiny, white);

    render_text("Last played:", x1, y + 250, font_tiny, green);
    render_text(rom.last, x2, y + 250, font_tiny, white);

    render_text("System:", x1, y + 300, font_tiny, green);
    render_text(rom.system, x2, y + 300, font_tiny, white);

    render_text("Completed:", x1, y + 350, font_tiny, green);
    render_text(rom.completed ? "Yes" : "No", x2, y + 350, font_tiny, white);

    // Bottom file path.

    render_text("File:", x0, y + 595, font_mini, green);
    render_text(rom.file, x0 + 50, y + 595, font_mini, white);

    // Footer keybinds.
    multi_color_line = {{"A: ", yellow}, {"Launch", white}, {"  B: ", yellow}, {"Return", white}};

    if (!rom.video.empty()) {
        multi_color_line.push_back({"  Y: ", yellow});
        multi_color_line.push_back({"Video", white});
    }

    if (!rom.manual.empty()) {
        multi_color_line.push_back({"  X: ", yellow});
        multi_color_line.push_back({"Manual", white});
    }

    multi_color_line.push_back({"  Start: ", yellow});
    multi_color_line.push_back({"Sort by (", white});
    multi_color_line.push_back({sort_names[sort_by], green});
    multi_color_line.push_back({")", white});

    render_multicolor_text(multi_color_line, x0, SCREEN_HEIGHT - 35, font_mini);

    SDL_RenderPresent(renderer);
}

void GUI::handle_inputs()
{
    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    Rom&      rom = filtered_roms_list[selected_index];
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
            if (!in_game_detail) scroll_finished = true;
        } else if (e.type == SDL_JOYBUTTONDOWN) {
            switch (e.jbutton.button) {
            case 0: // B (b0)
                if (in_game_detail && roms_list.size() > 1) {
                    in_game_detail = false;
                    filter();
                } else {
                    is_running = false;
                }
                break;
            case 1: // A (b1)
                if (in_game_detail) {
                    launch_external(
                        "/mnt/SDCARD/Emus/" + rom.system + "/default.sh \"" + rom.file + "\"");
                } else {
                    in_game_detail = true;
                    scroll_finished = true;
                }
                break;
            case 2: // Y (b2)
                if (in_game_detail && !rom.video.empty()) {
                    launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
                }
                break;
            case 3: // X (b3)
                if (in_game_detail && !rom.manual.empty()) {
                    launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
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
                if (in_game_detail) {
                    switch_completed();
                } else {
                    filter_completed = (filter_completed + 1) % 3;
                    filter();
                }
                break;
            case 7: // Start button (b7)
                if (!in_game_detail) {
                    if (sort_by == e_last) {
                        sort_by = e_name;
                    } else {
                        sort_by = static_cast<Sort>(sort_by + 1);
                    }
                    sort();
                }
                break;
            case 8: // Guide button (Home) (b8)
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
            case SDLK_v:
                if (in_game_detail && !rom.video.empty()) {
                    launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
                }
                break;
            case SDLK_m:
                if (in_game_detail && !rom.manual.empty()) {
                    launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
                }
                break;
            case SDLK_c:
                if (in_game_detail) {
                    switch_completed();
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
                if (in_game_detail) {
                    launch_external(
                        "/mnt/SDCARD/Emus/" + rom.system + "/default.sh \"" + rom.file + "\"");
                } else {
                    in_game_detail = true;
                    scroll_finished = true;
                }
                break;
            case SDLK_ESCAPE:
                if (in_game_detail && roms_list.size() > 1) {
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
    if (prev_selected_index != selected_index) {
        scroll_finished = true;
    }
}

void GUI::run(const std::string& rom_name)
{
    load_config_file();
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
