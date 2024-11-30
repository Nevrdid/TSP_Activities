#include "App.h"

App::App(const std::string& rom_name)
    : cfg()
    , gui(cfg)
{

    gui.init();
    if (TTF_Init()) {
        std::cerr << "Failed to init ttf: " << TTF_GetError() << std::endl;
        return;
    }

    std::string font = cfg.theme_path + cfg.theme.font;

    font_big = TTF_OpenFont(font.c_str(), FONT_BIG_SIZE);
    font_middle = TTF_OpenFont(font.c_str(), FONT_MIDDLE_SIZE);
    font_tiny = TTF_OpenFont(font.c_str(), FONT_TINY_SIZE);
    font_mini = TTF_OpenFont(font.c_str(), FONT_MINI_SIZE);

    if (!font_tiny || !font_middle || !font_big || !font_mini) {
        std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
        return;
    }

    DB db;
    if (rom_name == "") {
        roms_list = db.load_all();
        if (roms_list.empty())
            return;

        std::set<std::string> unique_systems;
        for (const auto& rom : roms_list) {
            unique_systems.insert(rom.system);
        }
        systems.push_back("");
        systems.insert(systems.end(), unique_systems.begin(), unique_systems.end());
        filter_roms();
    } else {
        no_list = true;
        in_game_detail = true;
        roms_list.push_back(db.load(rom_name));
        filtered_roms_list = roms_list;
    }
    is_running = true;
}

App::~App()
{

    TTF_CloseFont(font_mini);
    TTF_CloseFont(font_tiny);
    TTF_CloseFont(font_middle);
    TTF_CloseFont(font_big);
}

void App::switch_completed()
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

void App::filter_roms()
{
    const std::string& current_system = systems[system_index];

    filtered_roms_list.clear();
    for (const auto& rom : roms_list) {
        if (rom.system == current_system || system_index == 0) {
            if (filter_completed == 0)
                filtered_roms_list.push_back(rom);
            else if (filter_completed == 1 && rom.completed == 1)
                filtered_roms_list.push_back(rom);
            else if (filter_completed == 2 && rom.completed == 0)
                filtered_roms_list.push_back(rom);
        }
    }
    list_size = filtered_roms_list.size();
    sort_roms();

    if (selected_index >= filtered_roms_list.size())
        selected_index = filtered_roms_list.empty() ? 0 : filtered_roms_list.size() - 1;
}

void App::sort_roms()
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
    gui.reset_scroll();
}

const SDL_Color& white = {255,255,255,255};
void             App::game_list()
{
    gui.render_text(completed_names[filter_completed] + " " + systems[system_index] + " Games",
        X_0 + 75, -5, font_big, cfg.secondary_color);

    size_t first = (list_size <= LIST_LINES)
                       ? 0
                       : std::max(0, static_cast<int>(selected_index) - LIST_LINES / 2);
    size_t last = std::min(first + LIST_LINES, filtered_roms_list.size());

    int y = Y_0 + 70;
    for (size_t j = first; j < last; ++j) {
        const Rom& rom = filtered_roms_list[j];
        SDL_Color  color = (j == selected_index) ? cfg.primary_color : white;

        if (j == selected_index)
            gui.render_scrollable_text(rom.name, X_0, y, 840, font_middle, color);
        else
            gui.render_text(rom.name, X_0, y, font_middle, color, 840);

        y += Y_LINE / 2 + 10;
        gui.render_multicolor_text(
            {{"Time: ", cfg.secondary_color}, {rom.total_time, color},
                {"  Count: ", cfg.secondary_color}, {std::to_string(rom.count), color},
                {"  Last: ", cfg.secondary_color}, {rom.last, color}},
            X_0 + 10, y, font_tiny);

        y += Y_LINE / 2 - 10;
    }
    gui.render_image(filtered_roms_list[selected_index].image, X_0 + 1059, 370, 399, 0);

    gui.render_multicolor_text(
        {{"A: ", cfg.secondary_color}, {"Select", white}, {"  B: ", cfg.secondary_color},
            {"Quit", white}, {"  L/R: ", cfg.secondary_color}, {"Change system", white},
            {"  Select: ", cfg.secondary_color}, {"Filter (Un)Completed", white},
            {"  Start: ", cfg.secondary_color}, {"Sort by (", white},
            {sort_names[sort_by], cfg.primary_color}, {")", white}},
        X_0, cfg.height - 35, font_mini);

    gui.render();
}

void App::game_detail()
{
    const Rom& rom = filtered_roms_list[selected_index];

    // Header: Game name
    gui.render_scrollable_text(rom.name, 80, -5, 1120, font_middle, cfg.secondary_color);

    // Left side: Rom image
    gui.render_image(rom.image, 300, 350, 0, 560, true);

    // Right side: Game details
    int y = Y_0 + 150;
    int x0 = X_0 + 25;
    int x1 = x0 + 600;
    int x2 = x1 + 230;

    std::vector<std::pair<std::string, std::string>> details = {{"Total Time: ", rom.total_time},
        {"Average Time: ", rom.average_time}, {"Play count: ", std::to_string(rom.count)},
        {"Last played: ", rom.last}, {"System: ", rom.system},
        {"Completed: ", rom.completed ? "Yes" : "No"}};

    for (const auto& [label, value] : details) {
        gui.render_text(label, x1, y, font_tiny, cfg.primary_color);
        gui.render_text(value, x2, y, font_tiny, white);
        y += 50;
    }

    // Bottom: File path.
    gui.render_text("File:", x0, y + 195, font_mini, cfg.primary_color);
    gui.render_text(rom.file, x0 + 50, y + 195, font_mini, white);

    // Footer keybinds.
    std::vector<std::pair<std::string, SDL_Color>> multi_color_line = {{"A: ", cfg.secondary_color},
        {"Launch", white}, {"  B: ", cfg.secondary_color}, {"Return", white}};

    if (!rom.video.empty()) {
        multi_color_line.emplace_back("  Y: ", cfg.secondary_color);
        multi_color_line.emplace_back("Video", white);
    }
    if (!rom.manual.empty()) {
        multi_color_line.emplace_back("  X: ", cfg.secondary_color);
        multi_color_line.emplace_back("Manual", white);
    }

    multi_color_line.emplace_back("  Start: ", cfg.secondary_color);
    multi_color_line.emplace_back("Sort by (", white);
    multi_color_line.emplace_back(sort_names[sort_by], cfg.primary_color);
    multi_color_line.emplace_back(")", white);

    gui.render_multicolor_text(multi_color_line, x0, cfg.height - 35, font_mini);
    gui.render();
}

void App::empty_db()
{
    gui.render_background();

    gui.render_text("No datas available", 50, cfg.height / 2, font_big, white);

    gui.render();
}

void App::handle_inputs()
{
    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    Rom&      rom = filtered_roms_list[selected_index];
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_JOYHATMOTION:
            switch (e.jhat.value) {
            case SDL_HAT_UP: // D-Pad Up
                if (!in_game_detail && selected_index > 0)
                    selected_index--;
                break;
            case SDL_HAT_DOWN: // D-Pad Down
                if (!in_game_detail && selected_index < filtered_roms_list.size() - 1)
                    selected_index++;
                break;
            case SDL_HAT_LEFT: // D-Pad Left
                if (!in_game_detail)
                    selected_index = selected_index > 9 ? selected_index - 10 : 0;
                break;
            case SDL_HAT_RIGHT: // D-Pad Right
                if (!in_game_detail)
                    selected_index = list_size > 10 && selected_index < list_size - 10
                                         ? selected_index + 10
                                         : list_size - 1;
                break;
            }
            if (!in_game_detail)
                gui.reset_scroll();
            break;
        case SDL_JOYBUTTONDOWN:
            switch (e.jbutton.button) {
            case 0: // B (b0)
                if (in_game_detail && !no_list) {
                    in_game_detail = false;
                    filter_roms();
                } else {
                    is_running = false;
                }
                break;
            case 1: // A (b1)
                if (in_game_detail) {
                    gui.launch_external(
                        "/mnt/SDCARD/Emus/" + rom.system + "/default.sh \"" + rom.file + "\"");
                } else {
                    in_game_detail = true;
                    gui.reset_scroll();
                }
                break;
            case 2: // Y (b2)
                if (in_game_detail && !rom.video.empty()) {
                    gui.launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
                }
                break;
            case 3: // X (b3)
                if (in_game_detail && !rom.manual.empty()) {
                    gui.launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
                }
                break;
            case 4: // L1 (b4)
                if (!systems.empty()) {
                    system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
                    filter_roms();
                }
                break;
            case 5: // R1 (b5)
                if (!systems.empty()) {
                    system_index = (system_index + 1) % systems.size();
                    filter_roms();
                }
                break;
            case 6: // Back button (b6)
                if (in_game_detail) {
                    switch_completed();
                } else {
                    filter_completed = (filter_completed + 1) % 3;
                    filter_roms();
                }
                break;
            case 7: // Start button (b7)
                if (!in_game_detail) {
                    if (sort_by == e_last) {
                        sort_by = e_name;
                    } else {
                        sort_by = static_cast<Sort>(sort_by + 1);
                    }
                    sort_roms();
                }
                break;
            case 8: // Guide button (Home) (b8)
                break;
            case 9: // Left Stick Button (L3) (b9)
                break;
            case 10: // Right Stick Button (R3) (b10)
                break;
            }
            break;
#if defined(USE_KEYBOARD)
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym) {
            case SDLK_UP:
                if (!in_game_detail && selected_index > 0)
                    selected_index--;
                break;
            case SDLK_DOWN:
                if (!in_game_detail && selected_index < filtered_roms_list.size() - 1)
                    selected_index++;
                break;
            case SDLK_LEFT:
                if (!in_game_detail)
                    selected_index = selected_index > 10 ? selected_index - 10 : 0;
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
                    sort_roms();
                }
                break;
            case SDLK_v:
                if (in_game_detail && !rom.video.empty()) {
                    gui.launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
                }
                break;
            case SDLK_m:
                if (in_game_detail && !rom.manual.empty()) {
                    gui.launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
                }
                break;
            case SDLK_c:
                if (in_game_detail) {
                    switch_completed();
                } else {
                    filter_completed = (filter_completed + 1) % 3;
                    filter_roms();
                }
                break;
            case SDLK_g:
                if (!systems.empty()) {
                    system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
                    filter_roms();
                }
                break;
            case SDLK_h:
                if (!systems.empty()) {
                    system_index = (system_index + 1) % systems.size();
                    filter_roms();
                }
                break;
            case SDLK_RETURN:
                if (in_game_detail) {
                    gui.launch_external(
                        "/mnt/SDCARD/Emus/" + rom.system + "/default.sh \"" + rom.file + "\"");
                } else {
                    in_game_detail = true;
                    gui.reset_scroll();
                }
                break;
            case SDLK_ESCAPE:
                if (in_game_detail && !no_list) {
                    in_game_detail = false;
                    filter_roms();
                } else {
                    is_running = false;
                }
                break;
            }
            break;
#endif
        }
    }
    if (prev_selected_index != selected_index)
        gui.reset_scroll();
}

void App::run()
{
    std::cout << "ActivitiesApp: Starting GUI" << std::endl;
    if (!is_running) {
        empty_db();
        sleep(5);
    }
    while (is_running) {
        handle_inputs();
        if (in_game_detail) {
            gui.render_background(systems[system_index]);
            gui.render_overlay(DETAILS_OVERLAY);
            game_detail();
        } else {
            gui.render_background(systems[system_index]);
            gui.render_overlay(LIST_OVERLAY);
            game_list();
        }
        SDL_Delay(16); // ~60 FPS
    }
}
