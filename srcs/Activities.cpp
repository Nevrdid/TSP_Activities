#include "Activities.h"

Activities::Activities()
    : cfg()
    , gui(cfg)
{
    gui.init();
    is_running = true;
    refresh_db();
}

Activities::~Activities()
{
    gui.clean();
}

void Activities::switch_completed()
{
    // Safety check
    if (filtered_roms_list.empty() ||
        selected_index >= filtered_roms_list.size()) {
        std::cerr << "Error: Invalid ROM access in switch_completed()" << std::endl;
        return;
    }

    Rom& rom = *filtered_roms_list[selected_index];

    rom.completed = rom.completed ? 0 : 1;
    DB db;
    rom = db.save(rom);
}

void Activities::filter_roms()
{
    // Safety check to avoid out-of-bounds access
    std::string current_system = "";
    if (!systems.empty() && system_index < systems.size()) {
        current_system = systems[system_index];
    }

    filtered_roms_list.clear();
    total_time = 0;
    for (auto it = roms_list.begin(); it != roms_list.end(); it++) {
        if (it->system == current_system || system_index == 0) {
            if (filter_state == 0 || (filter_state == 1 && it->pid != -1) ||
                (filter_state == 2 && it->completed == 1) ||
                (filter_state == 3 && it->completed == 0)) {
                filtered_roms_list.push_back(it);
                total_time += it->time;
            }
        }
    }
    list_size = filtered_roms_list.size();

    sort_roms();

    if (selected_index >= list_size)
        selected_index = list_size == 0 ? 0 : static_cast<int>(list_size - 1);
}

void Activities::sort_roms()
{
    switch (sort_by) {
    case Sort::Name:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                return a->name < b->name;
            });
        break;
    case Sort::Time:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                return a->total_time > b->total_time;
            });
        break;
    case Sort::Count:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                return a->count > b->count;
            });
        break;
    case Sort::Last:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                return a->last > b->last;
            });
        break;
    }
    gui.reset_scroll();
}

void Activities::game_list()
{
    Vec2 prevSize;

    gui.render_image(cfg.theme_path + "skin/title-bg.png", gui.Width / 2, FONT_MIDDLE_SIZE,
        gui.Width, 2 * FONT_MIDDLE_SIZE);

    // Display the current/total index at the bottom right
    if (list_size > 0) {
        std::string index_text =
            std::to_string(selected_index + 1) + "/" + std::to_string(list_size);
        // Calculate text width to position it properly from the right edge
        int text_width =
            index_text.length() * (FONT_TINY_SIZE * 0.7); // Approximate character width
        gui.render_text(index_text, gui.Width - text_width - 10,
            gui.Height - FONT_MINI_SIZE * 2.5 - 50, FONT_TINY_SIZE, cfg.unselect_color);
    }
    // Centered title
    std::string current_system_name = "";
    if (!systems.empty() && system_index < systems.size()) {
        current_system_name = systems[system_index];
    }
    gui.render_text(states_names[filter_state] + " " + current_system_name + " Games",
        gui.Width / 2 - 100, 0, FONT_MIDDLE_SIZE, cfg.unselect_color);

    // Total time to the right of the title
    gui.render_text("(" + utils::stringifyTime(total_time) + ")", gui.Width - 2 * FONT_MIDDLE_SIZE,
        0.4 * FONT_MIDDLE_SIZE, FONT_TINY_SIZE, cfg.unselect_color, 0, true);
    size_t first = (list_size <= LIST_LINES)
                       ? 0
                       : std::max(0, static_cast<int>(selected_index) - LIST_LINES / 2);
    size_t last = std::min(first + LIST_LINES, filtered_roms_list.size());

    int y = 80;
    int x = 10;
    for (size_t j = first; j < last; ++j) {
        std::vector<Rom>::iterator rom = filtered_roms_list[j];
        SDL_Color                  color =
            (j == selected_index) ? cfg.selected_color : cfg.unselect_color;

        if (j == selected_index) {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-f.png", x, y, 0, 0, IMG_NONE);
        } else {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-n.png", x, y, 0, 0, IMG_NONE);
        }

        // Green dot if game is running (pid != -1)
        size_t name_offset = 15;

        if (rom->completed)
            gui.render_image(
                std::string(APP_DIR) + "/.assets/green_check.svg", x, y, 16, 16, IMG_NONE);
        if (rom->pid != -1)
            name_offset += gui.render_image(std::string(APP_DIR) + "/.assets/green_dot.svg", x + 16,
                                  y, 16, 16, IMG_NONE)
                               .x;

        // Display game name (accounting for the icons)
        if (j == selected_index) {
            gui.render_scrollable_text(rom->name, x + name_offset, y + 2,
                prevSize.x - 5 - name_offset, FONT_MIDDLE_SIZE, color);
        } else {
            gui.render_text(rom->name, x + name_offset, y + 2, FONT_MIDDLE_SIZE, color,
                prevSize.x - 5 - name_offset);
        }

        gui.render_multicolor_text(
            {{"Time: ", cfg.unselect_color}, {rom->total_time, color},
                {"  Count: ", cfg.unselect_color}, {std::to_string(rom->count), color},
                {"  Last: ", cfg.unselect_color}, {rom->last, color}},
            x + 15, y + prevSize.y / 2 + 6, FONT_TINY_SIZE);

        y += prevSize.y + 8;
    }
    if (list_size && gui.Width == 1280 &&
        selected_index < filtered_roms_list.size()) {
        gui.render_image(cfg.theme_path + "skin/ic-game-580.png", 1070, 370, 400, 580);
        gui.render_image(filtered_roms_list[selected_index]->image, 1070, 370, 400, 0);
    }

    gui.render_image(cfg.theme_path + "skin/tips-bar-bg.png", gui.Width / 2, gui.Height - 20,
        gui.Width, FONT_MINI_SIZE * 2);
    gui.display_keybind("A", "Y", "Run", 25);
    gui.display_keybind("B", "Exit", 145);
    gui.display_keybind("X", "Details", 230);
    gui.display_keybind("Menu", "Menu", 330);
    gui.display_keybind("L1", "R1", "Change system", gui.Width - 620);
    // gui.display_keybind("L2", "Manual", gui.Width - 500);
    gui.display_keybind("Select", "State filter", gui.Width - 350);
    gui.display_keybind("Start", "Sort by (" + sort_names[sort_by] + ")", gui.Width - 180);

    gui.render();

    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // Détection du relâchement du hat pour stopper l'auto-repeat
        if (e.type == SDL_JOYHATMOTION && e.jhat.value == SDL_HAT_CENTERED) {
            upHolding = false;
            downHolding = false;
        }
        InputAction action = gui.map_input(e);
        // Make sure filtered_roms_list is not empty before accessing selected_index
        const bool has_rom = !filtered_roms_list.empty() &&
                             selected_index < filtered_roms_list.size();
        const Rom& rom = has_rom ? *filtered_roms_list[selected_index] : *(Rom*) nullptr;
        switch (action) {
        case InputAction::Quit: is_running = false; break;
        case InputAction::Up: {
            if (selected_index > 0)
                selected_index--;
            // Start auto-scroll upwards
            upHolding = true;
            downHolding = false;
            holdStartTime = lastRepeatTime = std::chrono::steady_clock::now();
            break;
        }
        case InputAction::Down: {
            if (selected_index <list_size - 1)
                selected_index++;
            // Start auto-scroll downwards
            downHolding = true;
            upHolding = false;
            holdStartTime = lastRepeatTime = std::chrono::steady_clock::now();
            break;
        }
        case InputAction::Left:
            selected_index = selected_index > 10 ? selected_index - 10 : 0;
            upHolding = downHolding = false; // stop repeat on jump navigation
            break;
        case InputAction::Right:
            selected_index = list_size > 10 && selected_index < list_size - 10
                                 ? selected_index + 10
                                 : static_cast<int>(list_size) - 1;
            upHolding = downHolding = false;
            break;
        case InputAction::L1:
            system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
            filter_roms();
            upHolding = downHolding = false;
            break;
        case InputAction::R1:
            system_index = (system_index + 1) % systems.size();
            filter_roms();
            upHolding = downHolding = false;
            break;
        case InputAction::B: is_running = false; break;
        case InputAction::A:
            if (has_rom) {
                gui.launch_game(rom.name, rom.system, rom.file);
                pid_t ret = gui.wait_game(rom.name);
                filtered_roms_list[selected_index]->pid = ret;
                if (ret == -1 || ret == 0)
                    need_refresh = true;
            }
            break;
        case InputAction::X:
            in_game_detail = true;
            gui.reset_scroll();
            break;
        case InputAction::ZR:
            if (has_rom && !rom.manual.empty())
                gui.launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
            break;

        case InputAction::Select:
            filter_state = (filter_state + 1) % 4;
            filter_roms();
            upHolding = downHolding = false;
            break;
        case InputAction::Start:
            sort_by = sort_by == Sort::Last ? Sort::Name : static_cast<Sort>(sort_by + 1);
            sort_roms();
            upHolding = downHolding = false;
            break;
        case InputAction::Y: {
            // Y runs the game (same as A)
            if (has_rom) {
                gui.launch_game(rom.name, rom.system, rom.file);
                pid_t ret = gui.wait_game(rom.name);
                filtered_roms_list[selected_index]->pid = ret;
                if (ret == -1 || ret == 0)
                    need_refresh = true;
            }
            break;
        }
        case InputAction::Menu: {
            // Overlay menu: Complete/Uncomplete, Remove, Global stats, Exit
            if (!has_rom)
                break;
            bool menu_running = true;
            int  menu_index = 0; // 0: Complete/Uncomplete, 1: Remove, 2: Global stats, 3: Exit
            while (menu_running) {
                gui.load_background_texture();
                gui.render_image(cfg.theme_path + "skin/float-win-mask.png", gui.Width / 2,
                    gui.Height / 2, gui.Width, gui.Height);
                gui.render_image(
                    cfg.theme_path + "skin/pop-bg.png", gui.Width / 2, gui.Height / 2, 0, 0);

                int         centerY = gui.Height / 2;
                int         ys[4] = {centerY - 90, centerY - 30, centerY + 30, centerY + 90};
                const char* labels[4] = {
                    rom.completed ? "Uncomplete" : "Complete", "Remove", "Global stats", "Exit"};
                for (int i = 0; i < 4; ++i) {
                    Vec2 btnSize = gui.render_image(
                        cfg.theme_path + "skin/btn-bg-" +
                            (menu_index == i ? std::string("f") : std::string("n")) + ".png",
                        gui.Width / 2, ys[i]);
                    gui.render_text(labels[i], gui.Width / 2, ys[i] - btnSize.y / 2,
                        FONT_MIDDLE_SIZE, menu_index == i ? cfg.selected_color : cfg.unselect_color,
                        0, true);
                }

                gui.render();

                SDL_Event me;
                while (SDL_PollEvent(&me)) {
                    switch (gui.map_input(me)) {
                    case InputAction::Up: menu_index = (menu_index + 3) % 4; break;
                    case InputAction::Down: menu_index = (menu_index + 1) % 4; break;
                    case InputAction::B: menu_running = false; break;
                    case InputAction::Quit:
                        is_running = false;
                        menu_running = false;
                        break;
                    case InputAction::A: {
                        if (menu_index == 0) {
                            switch_completed();
                            upHolding = downHolding = false;
                        } else if (menu_index == 1) {
                            if (gui.confirmation_popup("Remove game from DB?", FONT_MIDDLE_SIZE)) {
                                DB db;
                                db.remove(rom.file);
                                roms_list.erase(
                                    std::remove_if(roms_list.begin(), roms_list.end(),
                                        [&rom](const Rom& r) { return r.file == rom.file; }),
                                    roms_list.end());
                                filter_roms();
                                if (filtered_roms_list.empty()) {
                                    is_running = false;
                                } else if (selected_index >=
                                           filtered_roms_list.size()) {
                                    selected_index =
                                        static_cast<int>(filtered_roms_list.size()) - 1;
                                }
                            }
                            upHolding = downHolding = false;
                        } else if (menu_index == 2) {
                            overall_stats();
                            upHolding = downHolding = false;
                        } else if (menu_index == 3) {
                            is_running = false;
                            upHolding = downHolding = false;
                        }
                        menu_running = false;
                        break;
                    }
                    default: break;
                    }
                }
            }
            gui.unload_background_texture();
            break;
        }
        default: break;
        }
    }

    // Auto-repeat handling (continuous scrolling)
    if ((upHolding || downHolding) && list_size > 0) {
        auto now = std::chrono::steady_clock::now();
        auto heldFor =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - holdStartTime).count();
        if (heldFor >= initialRepeatDelayMs) {
            auto sinceLast =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRepeatTime).count();
            if (sinceLast >= listRepeatIntervalMs) {
                if (upHolding && selected_index > 0) {
                    selected_index--;
                    lastRepeatTime = now;
                } else if (downHolding && selected_index < list_size - 1) {
                    selected_index++;
                    lastRepeatTime = now;
                }
            }
        }
    }

    if (prev_selected_index != selected_index)
        gui.reset_scroll();
}

void Activities::game_detail()
{

    // Safety check
    if (filtered_roms_list.empty() ||
        selected_index >= filtered_roms_list.size()) {
        std::cerr << "Error: Invalid ROM access in game_detail()" << std::endl;
        in_game_detail = false;
        return;
    }

    std::vector<Rom>::iterator rom = filtered_roms_list[selected_index];

    // Header: Game name
    gui.render_image(cfg.theme_path + "skin/title-bg.png", gui.Width / 2, FONT_MIDDLE_SIZE,
        gui.Width, 2 * FONT_MIDDLE_SIZE);
    gui.render_scrollable_text(
        rom->name, 10, 0, gui.Width - 2 * 10, FONT_MIDDLE_SIZE, cfg.unselect_color);

    // Left side: Rom image (reduced size)
    int frameWidth = gui.Width / 2 - 120;  // reduced
    int frameHeight = gui.Width / 2 - 120; // keep square
    int frameX = gui.Width / 4;
    int frameY = gui.Height / 2 - 30; // slightly adjusted upward
    gui.render_image(
        cfg.theme_path + "skin/bg-menu-09.png", frameX, frameY, frameWidth, frameHeight);
    if (!rom->image.empty()) {
        int innerW = frameWidth - 40;
        gui.render_image(rom->image, frameX, frameY, 0, innerW, IMG_FIT | IMG_CENTER);
    } else {
        gui.render_image(cfg.theme_path + "skin/ic-keymap-n.png", frameX, frameY);
    }

    // Dots navigation bar (chronologie: plus récent à droite) avec limite 20
    if (filtered_roms_list.size() > 1) {
        size_t    n = filtered_roms_list.size();
        const int maxDots = 20;
        size_t       displayCount = std::min(n, static_cast<size_t>(maxDots));
        int       dotsRadius = 6;
        int       spacing = 24;
        int       totalDotsWidth = (displayCount - 1) * spacing;
        int       rightX = frameX + totalDotsWidth / 2;
        int       dotsY = frameY + frameHeight / 2 + frameHeight / 2 - 10;
        if (dotsY > gui.Height - 200)
            dotsY = frameY + frameHeight / 2 + 40;
        for (size_t i = 0; i < displayCount; ++i) {
            int       xPos = rightX - i * spacing;
            bool      inRangeSelected = (selected_index == i);
            bool      filled = inRangeSelected;
            SDL_Color color = filled ? cfg.selected_color : cfg.unselect_color;
            gui.draw_circle(xPos, dotsY, dotsRadius, color, filled);
        }
        if (n > static_cast<size_t>(maxDots)) {
            // Leftmost displayed dot center
            int       xLeftmost = rightX - (displayCount - 1) * spacing;
            bool      highlightInfinity = selected_index >= maxDots;
            SDL_Color infColor = highlightInfinity ? cfg.selected_color : cfg.unselect_color;
            int       sep = dotsRadius * 2; // internal separation between the two infinity circles
            // Place right circle exactly one normal spacing to the left of leftmost dot
            int cxRight = xLeftmost - spacing;
            int cxLeft = cxRight - sep;
            // Draw (avoid going off-screen excessively)
            gui.draw_circle(cxLeft, dotsY, dotsRadius, infColor, highlightInfinity);
            gui.draw_circle(cxRight, dotsY, dotsRadius, infColor, highlightInfinity);
        }
    }

    // Right side: Game details
    std::vector<std::pair<std::string, std::string>> details = {
        {"Total Time: ", rom->total_time.empty() ? "N/A" : rom->total_time},
        {"Average Time: ", rom->average_time.empty() ? "N/A" : rom->average_time},
        {"Last played: ", rom->last.empty() ? "N/A" : rom->last},
        {"Last session: ", utils::stringifyTime(rom->lastsessiontime)},
        {"Play count: ", std::to_string(rom->count)},
        {"System: ", rom->system.empty() ? "N/A" : rom->system},
        {"Completed: ", rom->completed ? "Yes" : "No"}};
    gui.infos_window("Informations", FONT_TINY_SIZE, details, FONT_MINI_SIZE,
        3 * gui.Width / 4 - 10, gui.Height / 2, gui.Width / 2 - 50, gui.Height / 2);

    // Bottom: File path.
    gui.render_image(cfg.theme_path + "skin/list-item-2line-long-bg-n.png", gui.Width / 2,
        gui.Height - FONT_MINI_SIZE * 3.5, gui.Width - 2 * 10, FONT_MINI_SIZE * 2,
        IMG_FIT | IMG_CENTER);
    gui.render_text(
        "File:", 50, gui.Height - FONT_MINI_SIZE * 4.5 + 4, FONT_MINI_SIZE, cfg.selected_color);
    gui.render_text(
        rom->file, 100, gui.Height - FONT_MINI_SIZE * 4.5 + 4, FONT_MINI_SIZE, cfg.info_color);

    gui.render_image(cfg.theme_path + "skin/tips-bar-bg.png", gui.Width / 2, gui.Height - 20,
        gui.Width, FONT_MINI_SIZE * 2);
    gui.display_keybind("A", "Y", "Run", 25);
    gui.display_keybind("B", "Exit", 145);
    gui.display_keybind("X", "List", 230);
    gui.display_keybind("Menu", "Menu", 300);
    // gui.display_keybind("L2", "Manual", gui.Width / 2 - 80);
    // gui.display_keybind("Select", rom.completed ? "Uncomplete" : "Complete", gui.Width / 2);

    // Display navigation arrows on the screen sides if navigation is possible
    if (filtered_roms_list.size() > 1) {
        // Left arrow (newer elements)
        if (selected_index > 0) {
            gui.render_image(cfg.theme_path + "skin/ic-right-arrow-n.png", gui.Width - 10,
                gui.Height / 2, 40, 40);
        }
        // Right arrow (older elements)
        if (selected_index < filtered_roms_list.size() - 1) {
            gui.render_image(
                cfg.theme_path + "skin/ic-left-arrow-n.png", 10, gui.Height / 2, 40, 40);
        }

        int nav_x = gui.Width - 320;
        gui.render_image(
            cfg.theme_path + "skin/ic-left-arrow-n.png", nav_x, gui.Height - 20, 30, 30);
        gui.render_image(
            cfg.theme_path + "skin/ic-right-arrow-n.png", nav_x + 35, gui.Height - 20, 30, 30);
        gui.render_text("Navigate", nav_x + 50, gui.Height - 32, FONT_MINI_SIZE, cfg.info_color);
    }

    // Y now runs the game; remove Y Video hint
    if (!rom->manual.empty())
        gui.display_keybind("X", "Manual", gui.Width - 125);

    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // Stop auto-repeat when hat returns to center
        if (e.type == SDL_JOYHATMOTION && e.jhat.value == SDL_HAT_CENTERED) {
            leftHolding = false;
            rightHolding = false;
        }
        switch (gui.map_input(e)) {
        case InputAction::Quit: is_running = false; break;
        case InputAction::B: is_running = false; break;
        case InputAction::Left:
            if (selected_index < filtered_roms_list.size() - 1) {
                selected_index++;
                gui.reset_scroll();
            }
            // Start auto-scroll to newer items (Left)
            leftHolding = true;
            rightHolding = false;
            holdStartTime = lastRepeatTime = std::chrono::steady_clock::now();
            break;
        case InputAction::Right:
            if (selected_index > 0) {
                selected_index--;
                gui.reset_scroll();
            }
            // Start auto-scroll to older items (Right)
            rightHolding = true;
            leftHolding = false;
            holdStartTime = lastRepeatTime = std::chrono::steady_clock::now();
            break;
        case InputAction::A:
            gui.launch_game(rom->name, rom->system, rom->file);
            {
                pid_t ret = gui.wait_game(rom->name);
                filtered_roms_list[selected_index]->pid = ret;
                if (ret == -1 || ret == 0)
                    need_refresh = true;
            }
            break;
        case InputAction::Y:
            // Y runs the game now (same as A)
            gui.launch_game(rom->name, rom->system, rom->file);
            {
                pid_t ret = gui.wait_game(rom->name);
                filtered_roms_list[selected_index]->pid = ret;
                if (ret == -1 || ret == 0)
                    need_refresh = true;
            }
            break;
        case InputAction::ZL:
            if (!rom->video.empty())
                gui.launch_external(std::string(VIDEO_PLAYER) + " \"" + rom->video + "\"");
            break;
        case InputAction::X:
            in_game_detail = false;
            leftHolding = rightHolding = false;
            break;
        case InputAction::ZR:
            if (!rom->manual.empty())
                gui.launch_external(std::string(MANUAL_READER) + " \"" + rom->manual + "\"");
            break;
        case InputAction::Menu: {
            // Overlay menu: Complete/Uncomplete, Remove, Global stats, Exit
            bool menu_running = true;
            int  menu_index = 0; // 0: Complete/Uncomplete, 1: Remove, 2: Global stats, 3: Exit
            while (menu_running) {
                gui.load_background_texture();
                gui.render_image(cfg.theme_path + "skin/float-win-mask.png", gui.Width / 2,
                    gui.Height / 2, gui.Width, gui.Height);
                // Popup background
                gui.render_image(
                    cfg.theme_path + "skin/pop-bg.png", gui.Width / 2, gui.Height / 2, 0, 0);

                // Draw options as buttons (4 items)
                int         centerY = gui.Height / 2;
                int         ys[4] = {centerY - 90, centerY - 30, centerY + 30, centerY + 90};
                const char* labels[4] = {
                    rom->completed ? "Uncomplete" : "Complete", "Remove", "Global stats", "Exit"};
                for (int i = 0; i < 4; ++i) {
                    Vec2 btnSize = gui.render_image(
                        cfg.theme_path + "skin/btn-bg-" +
                            (menu_index == i ? std::string("f") : std::string("n")) + ".png",
                        gui.Width / 2, ys[i]);
                    gui.render_text(labels[i], gui.Width / 2, ys[i] - btnSize.y / 2,
                        FONT_MIDDLE_SIZE, menu_index == i ? cfg.selected_color : cfg.unselect_color,
                        0, true);
                }

                gui.render();

                SDL_Event me;
                while (SDL_PollEvent(&me)) {
                    switch (gui.map_input(me)) {
                    case InputAction::Up: menu_index = (menu_index + 3) % 4; break;   // up
                    case InputAction::Down: menu_index = (menu_index + 1) % 4; break; // down
                    case InputAction::B: menu_running = false; break;
                    case InputAction::Quit:
                        is_running = false;
                        menu_running = false;
                        break;
                    case InputAction::A: {
                        if (menu_index == 0) {
                            // Same as Select action (toggle complete)
                            switch_completed();
                            leftHolding = rightHolding = false;
                        } else if (menu_index == 1) {
                            // Same as Menu action (Remove)
                            if (gui.confirmation_popup("Remove game from DB?", FONT_MIDDLE_SIZE)) {
                                DB db;
                                db.remove(rom->file);
                                roms_list.erase(
                                    std::remove_if(roms_list.begin(), roms_list.end(),
                                        [&rom](const Rom& r) { return r.file == rom->file; }),
                                    roms_list.end());
                                filter_roms();
                                in_game_detail = false;
                            }
                            leftHolding = rightHolding = false;
                        } else if (menu_index == 2) {
                            // Global stats
                            overall_stats();
                            leftHolding = rightHolding = false;
                        } else if (menu_index == 3) {
                            // Exit
                            is_running = false;
                            leftHolding = rightHolding = false;
                        }
                        menu_running = false;
                        break;
                    }
                    default: break;
                    }
                }
            }
            gui.unload_background_texture();
            break;
        }
        case InputAction::Select:
            switch_completed();
            leftHolding = rightHolding = false;
            break;
        default: break;
        }
    }

    // Auto-repeat handling for left/right navigation in detail view
    if ((leftHolding || rightHolding) && filtered_roms_list.size() > 1) {
        auto now = std::chrono::steady_clock::now();
        auto heldFor =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - holdStartTime).count();
        if (heldFor >= initialRepeatDelayMs) {
            auto sinceLast =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRepeatTime).count();
            if (sinceLast >= detailRepeatIntervalMs) {
                if (leftHolding &&
                    selected_index < filtered_roms_list.size() - 1) {
                    selected_index++;
                    lastRepeatTime = now;
                } else if (rightHolding && selected_index > 0) {
                    selected_index--;
                    lastRepeatTime = now;
                }
            }
        }
    }

    if (prev_selected_index != selected_index) {
        gui.reset_scroll();
    }

    gui.render();
}

void Activities::overall_stats()
{
    bool running = true;
    while (is_running && running) {

        int count = 0;
        int completed = 0;
        int time = 0;
        for (const auto& rom : roms_list) {
            count += rom.count;
            time += rom.time;
            completed += rom.completed;
        }
        std::vector<std::pair<std::string, std::string>> content = {
            {"Total games: ", std::to_string(roms_list.size())},
            {"Total completed: ", std::to_string(completed)},
            {"Total play count: ", std::to_string(count)},
            {"Total play time: ", utils::stringifyTime(time)},
            {"Average play time: ", utils::stringifyTime(count ? time / count : 0)}};

        gui.load_background_texture();
        gui.render_image(cfg.theme_path + "skin/float-win-mask.png", gui.Width / 2, gui.Height / 2,
            gui.Width, gui.Height);
        gui.infos_window("Overall stats", FONT_MIDDLE_SIZE, content, FONT_TINY_SIZE, gui.Width / 2,
            gui.Height / 2, gui.Width / 2, gui.Height / 2);

        gui.render();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (gui.map_input(e)) {
            case InputAction::Quit: is_running = false; break;
            case InputAction::Menu:
            case InputAction::B:
            case InputAction::A: running = false; break;
            default: break;
            }
        }
    }
    gui.unload_background_texture();
}

void Activities::empty_db()
{
    gui.render_background();
    gui.render_text("No data available.", gui.Width / 2, gui.Height / 2, FONT_BIG_SIZE,
        cfg.title_color, 0, true);
    gui.render();
}

Rom* Activities::get_rom(const std::string& rom_file)
{
    auto it = roms_list.begin();
    do {
        if (it->file == rom_file || fs::path(it->file).filename() == fs::path(rom_file)) {
            std::cout << "ROM " << it->name << "found in database." << std::endl;

            return &(*it);
            // in_game_detail = true;
            // Is it needed out of gui init?
        }
    } while (it++ != roms_list.end());
    return nullptr;
}

void Activities::refresh_db(std::string selected_rom_file)
{
    // Save the current selected rom file (if any)
    if (selected_rom_file.empty()) {
        if (!filtered_roms_list.empty() &&
            selected_index < filtered_roms_list.size())
            selected_rom_file = filtered_roms_list[selected_index]->file;
    } else {
        selected_rom_file = utils::shorten_file_path(selected_rom_file);

        if (get_rom(selected_rom_file)) {
            std::cout << "ROM not found in database, creating new entry for: " << selected_rom_file
                      << std::endl;
            DB  db;
            Rom new_rom = db.save(selected_rom_file);
        }
    }

    // TODO: change DB as an instance
    DB db;
    roms_list = db.load();

    // Reinjects pids for games still running (present in gui.childs)
    auto& childs = gui.get_childs();
    for (auto& rom : roms_list) {
        auto it = childs.find(rom.name);
        if (it != childs.end()) {
            // Checks that the process still exists
            if (kill(it->second, 0) == 0) {
                rom.pid = it->second;
            } else {
                rom.pid = -1;
                childs.erase(it); // Cleans up dead pids
            }
        } else {
            rom.pid = -1;
        }
    }

    std::set<std::string> unique_systems;
    for (const auto& rom : roms_list) {
        unique_systems.insert(rom.system);
    }
    systems.push_back("");
    systems.insert(systems.end(), unique_systems.begin(), unique_systems.end());
    filter_roms();
    // Restore selection to the same rom if possible
    for (size_t i = 0; i < filtered_roms_list.size(); ++i) {
        if (filtered_roms_list[i]->file == selected_rom_file) {
            selected_index = i;
            break;
        }
    }
    if (selected_index >= filtered_roms_list.size())
        selected_index = 0;
    need_refresh = false;

    std::cout << "db_refreshed !! Selected_index: " << selected_index << std::endl;
    // // Debug: display loaded values
    // if (selected_index >= 0 && selected_index < static_cast<int>(filtered_roms_list.size())) {
    //     const Rom* loaded_rom = filtered_roms_list[selected_index];
    //     std::cout << "Final ROM data:" << std::endl;
    //     std::cout << "  Name: " << loaded_rom->name << std::endl;
    //     std::cout << "  File: " << loaded_rom->file << std::endl;
    //     std::cout << "  Count: " << loaded_rom->count << std::endl;
    //     std::cout << "  Time: " << loaded_rom->time << std::endl;
    //     std::cout << "  Total time: '" << loaded_rom->total_time << "'" << std::endl;
    //     std::cout << "  Average time: '" << loaded_rom->average_time << "'" << std::endl;
    //     std::cout << "  System: '" << loaded_rom->system << "'" << std::endl;
    //     std::cout << "  Last: '" << loaded_rom->last << "'" << std::endl;
    //     std::cout << "  Selected index: " << selected_index << std::endl;
    //     std::cout << "  Total ROMs loaded: " << roms_list.size() << std::endl;
    //     std::cout << "  Filtered ROMs: " << filtered_roms_list.size() << std::endl;
    // }
}

static const char gui_help[] = {
    "activities GUI usage:\n"
    "\tactivities gui [option...]*\n"
    "Options:\n"
    "  -h\tDisplay help text\n"
    "  -D\tStart the gui in details mode instead of list\n"
    "  -s\tSet initial sort method. Default: (name,time,count,\e[0mlast\e[1m)\n"
    "  -S\tSet the initial system to filter. (\e[1mall\e[0m,gba,psp,fc...)\n"
    "  -c\tSet the initial completed filter (\e[1mall\e[1m,on,off)\n"
    "  -f\tSet the initial romFile to display. default: first of the filtered and sorted list. "
    "\n"};

extern char* optarg;
extern int   optind;

static const char options[] = "hDs:S:c:f:";
int               Activities::parseArgs(int argc, char** argv)
{
    int option;
    int status = 0;

    std::string tmp_str;
    auto        sys_it = systems.begin();
    while ((option = getopt(argc, argv, options)) != -1) {
        switch (option) {
        case 'h':
            puts(gui_help);
            status = 1;
            break;
        case 'D': in_game_detail = 1; break;
        case 's':
            if (optarg == NULL)
                break;
            // sort_by is already set to Sort::Last
            tmp_str = std::string(optarg);
            if (tmp_str == "name") {
                sort_by = Sort::Name;
            } else if (tmp_str == "time") {
                sort_by = Sort::Time;
            } else if (tmp_str == "count") {
                sort_by = Sort::Count;
            }
            break;
        case 'S':
            if (optarg == NULL)
                break;
            tmp_str = std::string(optarg);

            // system_index is already set to 0 (all)
            sys_it = std::find(systems.begin(), systems.end(), tmp_str);
            if (sys_it != systems.end()) {
                system_index = std::distance(systems.begin(), sys_it);
            }
            break;
        case 'c':
            if (optarg == NULL)
                break;
            tmp_str = std::string(optarg);
            if (tmp_str == "all") {
                filter_state = 0;
            } else if (tmp_str == "on") {
                filter_state = 2;
            } else if (tmp_str == "off") {
                filter_state = 3;
            }
            break;
        case 'f':
            if (optarg == NULL)
                break;
            refresh_db(optarg);
            break;
        default: status = 1; break;
        }
    }

    return status;
}

void Activities::run(int argc, char** argv)
{
    if (parseArgs(argc, argv)) {
        puts(gui_help);
        exit(1);
    }

    std::cout << "ActivitiesApp: Starting GUI" << std::endl;
    if (!is_running) {
        empty_db();
        sleep(5);
    }
    while (is_running) {
        // Refreshes data after game return, with a one-second pause
        if (need_refresh) {
            if (!refresh_timer_active) {
                // Start the non-blocking timer
                refresh_timer_start = std::chrono::steady_clock::now();
                refresh_timer_active = true;
            } else {
                // Check if 1 second has elapsed
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - refresh_timer_start);

                if (elapsed.count() >= 1000) {
                    // 1 second has elapsed, proceed with refresh
                    refresh_timer_active = false;
                    refresh_db();
                }
            }
        }
        // Safety check to avoid out-of-bounds access
        std::string current_system = "";
        if (!systems.empty() && system_index < systems.size()) {
            current_system = systems[system_index];
        }
        gui.render_background(current_system);
        if (in_game_detail)
            game_detail();
        else
            game_list();
        SDL_Delay(16); // ~60 FPS
    }
}
