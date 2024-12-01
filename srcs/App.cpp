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
    case Sort::Name:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.name < b.name; });
        break;
    case Sort::Time:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.total_time > b.total_time; });
        break;
    case Sort::Count:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.count > b.count; });
        break;
    case Sort::Last:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](const Rom& a, const Rom& b) { return a.last > b.last; });
        break;
    }
    gui.reset_scroll();
}

const SDL_Color& white = {255, 255, 255, 255};

void App::game_list()
{
    Vec2 prevSize;
    prevSize = gui.render_image(cfg.theme_path + "skin/nav-logo.png", 10, 20, 0, 0, false, false);
    gui.render_text(completed_names[filter_completed] + " " + systems[system_index] + " Games",
        2 * cfg.list_x0 + prevSize.x, -7, font_big, cfg.secondary_color);

    size_t first = (list_size <= LIST_LINES)
                       ? 0
                       : std::max(0, static_cast<int>(selected_index) - LIST_LINES / 2);
    size_t last = std::min(first + LIST_LINES, filtered_roms_list.size());

    int y = cfg.list_y0;
    int i = 0;
    for (size_t j = first; j < last; ++j) {
        const Rom& rom = filtered_roms_list[j];
        SDL_Color  color = (j == selected_index) ? cfg.primary_color : white;

        if (j == selected_index) {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-f.png", cfg.list_mid, y);
            gui.render_scrollable_text(
                rom.name, cfg.list_x0, y - prevSize.y / 2, prevSize.x, font_middle, color);
        } else {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-n.png", cfg.list_mid, y);
            gui.render_text(
                rom.name, cfg.list_x0, y - prevSize.y / 2, font_middle, color, prevSize.x);
        }

        gui.render_multicolor_text(
            {{"Time: ", cfg.secondary_color}, {rom.total_time, color},
                {"  Count: ", cfg.secondary_color}, {std::to_string(rom.count), color},
                {"  Last: ", cfg.secondary_color}, {rom.last, color}},
            cfg.list_x0 + 10, y + 10, font_tiny);

        y += prevSize.y + cfg.list_dy;
        i++;
    }
    if (list_size && cfg.width == 1280) {
        gui.render_image(cfg.theme_path + "skin/ic-game-580.png", 1070, 370, 400, 580);
        gui.render_image(filtered_roms_list[selected_index].image, 1070, 370, 400, 0);
    }

    gui.display_keybind("A", "Select", 25, cfg.height - 20, font_mini, white);
    gui.display_keybind("B", "Quit", 140, cfg.height - 20, font_mini, white);
    gui.display_keybind("Menu", "Global stats", 230, cfg.height - 20, font_mini, white);
    gui.display_keybind(
        "L1", "R1", "Change system", cfg.width - 620, cfg.height - 20, font_mini, white);
    gui.display_keybind(
        "Select", "State filter", cfg.width - 350, cfg.height - 20, font_mini, white);
    gui.display_keybind("Start", "Sort by (" + sort_names[sort_by] + ")", cfg.width - 180,
        cfg.height - 20, font_mini, white);

    gui.render();

    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        InputAction action = gui.map_input(e);
        switch (action) {
        case InputAction::Up:
            if (selected_index > 0)
                selected_index--;
            break;
        case InputAction::Down:
            if (selected_index < filtered_roms_list.size() - 1)
                selected_index++;
            break;
        case InputAction::Left:
            selected_index = selected_index > 10 ? selected_index - 10 : 0;
            break;
        case InputAction::Right:
            selected_index = list_size > 10 && selected_index < list_size - 10 ? selected_index + 10
                                                                               : list_size - 1;
            break;
        case InputAction::L1:
            system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
            filter_roms();
            break;
        case InputAction::R1:
            system_index = (system_index + 1) % systems.size();
            filter_roms();
            break;
        case InputAction::B: is_running = false; break;
        case InputAction::A:
            in_game_detail = true;
            gui.reset_scroll();
            break;

        case InputAction::Select:
            filter_completed = (filter_completed + 1) % 3;
            filter_roms();
            break;
        case InputAction::Start:
            sort_by = sort_by == Sort::Last ? Sort::Name : static_cast<Sort>(sort_by + 1);
            sort_roms();
            break;
        case InputAction::Menu: overall_stats(); break;
        default: break;
        }
    }
    if (prev_selected_index != selected_index)
        gui.reset_scroll();
}

void App::game_detail()
{
    Vec2       prevSize;
    const Rom& rom = filtered_roms_list[selected_index];

    prevSize = gui.render_image(
        cfg.theme_path + "skin/nav-logo.png", cfg.details_x0, 20, 0, 0, false, false);

    // Header: Game name
    gui.render_scrollable_text(rom.name, 2 * cfg.details_x0 + prevSize.x, -7,
        cfg.width - prevSize.x - 2 * cfg.details_x0, font_big, cfg.secondary_color);

    // Left side: Rom image
    gui.render_image(cfg.theme_path + "skin/bg-menu-09.png", cfg.width / 4, cfg.height / 2,
        cfg.details_img_size + 10, cfg.details_img_size + 10);
    if (!rom.image.empty())
        gui.render_image(rom.image, cfg.width / 4, cfg.height / 2, 0, cfg.details_img_size, true);
    else
        gui.render_image(cfg.theme_path + "skin/ic-keymap-n.png", cfg.width / 4, cfg.height / 2);

    // Right side: Game details

    prevSize = gui.render_image(cfg.theme_path + "skin/bg-menu-09.png",
        3 * cfg.width / 4 - cfg.details_x0, cfg.height / 2, 520, 360);

    std::vector<std::pair<std::string, std::string>> details = {{"Total Time: ", rom.total_time},
        {"Average Time: ", rom.average_time}, {"Play count: ", std::to_string(rom.count)},
        {"Last played: ", rom.last}, {"System: ", rom.system},
        {"Completed: ", rom.completed ? "Yes" : "No"}};
    int                                              y = cfg.details_y0;
    for (const auto& [label, value] : details) {
        gui.render_multicolor_text({{label, cfg.primary_color}, {value, white}},
            3 * cfg.width / 4 - prevSize.x / 2, y, font_tiny);
        y += 50;
    }

    // Bottom: File path.
    gui.render_image(cfg.theme_path + "skin/list-item-1line-sort-bg-n.png", cfg.width / 2,
        cfg.details_y1 + 15, cfg.width - 2 * cfg.details_x0, 30, true);
    gui.render_text("File:", cfg.details_x0, cfg.details_y1, font_mini, cfg.primary_color);
    gui.render_text(rom.file, cfg.details_x0 + 50, cfg.details_y1, font_mini, white);

    gui.display_keybind("A", "Start", 25, cfg.height - 20, font_mini, white);
    gui.display_keybind("B", "Return", 140, cfg.height - 20, font_mini, white);
    gui.display_keybind("Menu", "Remove", cfg.width / 2 - 150, cfg.height - 20, font_mini, white);
    gui.display_keybind("Select", rom.completed ? "Uncomplete" : "Complete", cfg.width / 2,
        cfg.height - 20, font_mini, white);
    if (!rom.video.empty())
        gui.display_keybind("Y", "Video", cfg.width - 240, cfg.height - 20, font_mini, white);
    if (!rom.manual.empty())
        gui.display_keybind("X", "Manual", cfg.width - 125, cfg.height - 20, font_mini, white);

    gui.render();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (gui.map_input(e)) {
        case InputAction::B: in_game_detail = false; break;
        case InputAction::A:
            gui.launch_external(
                "/mnt/SDCARD/Emus/" + rom.system + "/default.sh \"" + rom.file + "\"");
            break;
        case InputAction::Y:
            if (!rom.video.empty())
                gui.launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
            break;
        case InputAction::X:
            if (!rom.manual.empty())
                gui.launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
            break;
        case InputAction::Select: switch_completed(); break;
        case InputAction::Menu:
            if (confirmation_popup("Remove game from DB?")) {
                DB db;
                db.remove(rom.file);
                roms_list.erase(std::remove_if(roms_list.begin(), roms_list.end(),
                                    [&rom](const Rom& r) { return r.file == rom.file; }),
                    roms_list.end());
                filter_roms();
                in_game_detail = false;
            }
            break;
        default: break;
        }
    }
}

void App::overall_stats()
{
    while (true) {
        gui.render_background();
        gui.render_text("Overall stats", cfg.width / 2, 30, font_big, cfg.primary_color, 0, true);

        int count = 0;
        int completed = 0;
        int time = 0;
        for (const auto& rom : roms_list) {
            count += rom.count;
            time += rom.time;
            completed += rom.completed;
        }
        int         average = count ? time / count : 0;
        std::string total_time = utils::sec2hhmmss(time);
        std::string average_time = utils::sec2hhmmss(average);

        gui.render_text("Total games: " + std::to_string(roms_list.size()), cfg.width / 2, 150,
            font_middle, white, 0, true);
        gui.render_text("Total completed: " + std::to_string(completed), cfg.width / 2, 250,
            font_middle, white, 0, true);
        gui.render_text("Total play count: " + std::to_string(count), cfg.width / 2, 350,
            font_middle, white, 0, true);
        gui.render_text(
            "Total play time: " + total_time, cfg.width / 2, 450, font_middle, white, 0, true);
        gui.render_text(
            "Average play time: " + average_time, cfg.width / 2, 550, font_middle, white, 0, true);

        gui.render();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (gui.map_input(e)) {
            case InputAction::B: return; break;
            case InputAction::A: return; break;
            default: break;
            }
        }
    }
}

bool App::confirmation_popup(const std::string& message)
{
    bool confirmed = false;
    Vec2 prevSize;

    while (true) {
        gui.render_background();
        gui.render_text(message, cfg.width / 2, cfg.height / 3, font_middle, white, 0, true);

        prevSize = gui.render_image(
            cfg.theme_path + "skin/bg-button-02-" + (confirmed ? "selected" : "unselect") + ".png",
            cfg.width / 3, 2 * cfg.height / 3);
        gui.render_text("Yes", cfg.width / 3, 2 * cfg.height / 3 - prevSize.y / 2, font_middle,
            confirmed ? cfg.primary_color : white, 0, true);
        prevSize = gui.render_image(
            cfg.theme_path + "skin/bg-button-02-" + (confirmed ? "unselect" : "selected") + ".png",
            2 * cfg.width / 3, 2 * cfg.height / 3);
        gui.render_text("No", 2 * cfg.width / 3, 2 * cfg.height / 3 - prevSize.y / 2, font_middle,
            confirmed ? white : cfg.primary_color, 0, true);
        gui.render();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (gui.map_input(e)) {
            case InputAction::Left: confirmed = true; break;
            case InputAction::Right: confirmed = false; break;
            case InputAction::B: return false; break;
            case InputAction::A: return confirmed; break;
            default: break;
            }
        }
    }
}

void App::empty_db()
{
    gui.render_background();
    gui.render_text("No datas available.", cfg.width / 2, cfg.height / 2, font_big, white, 0, true);
    gui.render();
}

void App::run()
{
    std::cout << "ActivitiesApp: Starting GUI" << std::endl;
    if (!is_running) {
        empty_db();
        sleep(5);
    }
    while (is_running) {
        gui.render_background(systems[system_index]);
        if (in_game_detail)
            game_detail();
        else
            game_list();
        SDL_Delay(16); // ~60 FPS
    }
}
