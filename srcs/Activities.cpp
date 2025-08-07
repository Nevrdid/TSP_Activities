#include "Activities.h"

Activities::Activities(const std::string& rom_name)
    : cfg()
    , gui(cfg)
{

    gui.init();

    DB db;
    if (rom_name == "") {
        roms_list = db.load();
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
        
        // Load the entire database to allow navigation
        roms_list = db.load();
        
        // Normalize the searched ROM path
        std::string canonical_rom_name = rom_name;
        fs::path rom_path(rom_name);
        if (fs::exists(rom_path)) {
            canonical_rom_name = fs::canonical(rom_path);
        }
        
        // Search for the specific ROM in the complete list
        int target_rom_index = -1;
        for (size_t i = 0; i < roms_list.size(); ++i) {
            const auto& rom = roms_list[i];
            // Try multiple types of matching
            if (rom.file == rom_name || rom.file == canonical_rom_name || 
                fs::path(rom.file).filename() == fs::path(rom_name).filename()) {
                target_rom_index = i;
                std::cout << "ROM found in database at index " << i << ": " << rom.name << std::endl;
                break;
            }
        }
        
        // If ROM is not found in database, load/create it and add it
        if (target_rom_index == -1) {
            std::cout << "ROM not found in database, creating new entry for: " << rom_name << std::endl;
            Rom loaded_rom = db.load(rom_name);
            
            // Check that the ROM was correctly loaded
            if (loaded_rom.file.empty()) {
                std::cerr << "Failed to load ROM: " << rom_name << std::endl;
                is_running = false;
                // Initialize systems with a default value to avoid crashes
                systems.push_back("");
                return;
            }
            
            roms_list.push_back(loaded_rom);
            target_rom_index = roms_list.size() - 1;
        }
        
        // Build the list of unique systems
        std::set<std::string> unique_systems;
        for (const auto& rom : roms_list) {
            unique_systems.insert(rom.system);
        }
        systems.push_back("");
        systems.insert(systems.end(), unique_systems.begin(), unique_systems.end());
        
        // Filter ROMs
        filter_roms();
        
        // Find the target ROM index in the filtered list
        for (size_t i = 0; i < filtered_roms_list.size(); ++i) {
            if (static_cast<int>(i) < roms_list.size() && 
                roms_list[target_rom_index].file == filtered_roms_list[i].file) {
                selected_index = i;
                break;
            }
        }
        
        // Debug: display loaded values
        if (target_rom_index >= 0 && target_rom_index < static_cast<int>(roms_list.size())) {
            const Rom& loaded_rom = roms_list[target_rom_index];
            std::cout << "Final ROM data:" << std::endl;
            std::cout << "  Name: " << loaded_rom.name << std::endl;
            std::cout << "  File: " << loaded_rom.file << std::endl;
            std::cout << "  Count: " << loaded_rom.count << std::endl;
            std::cout << "  Time: " << loaded_rom.time << std::endl;
            std::cout << "  Total time: '" << loaded_rom.total_time << "'" << std::endl;
            std::cout << "  Average time: '" << loaded_rom.average_time << "'" << std::endl;
            std::cout << "  System: '" << loaded_rom.system << "'" << std::endl;
            std::cout << "  Last: '" << loaded_rom.last << "'" << std::endl;
            std::cout << "  Selected index: " << selected_index << std::endl;
            std::cout << "  Total ROMs loaded: " << roms_list.size() << std::endl;
            std::cout << "  Filtered ROMs: " << filtered_roms_list.size() << std::endl;
        }
    }
    is_running = true;
}

Activities::~Activities()
{
    gui.clean();
}

void Activities::switch_completed()
{
    // Safety check
    if (filtered_roms_list.empty() || selected_index >= static_cast<int>(filtered_roms_list.size())) {
        std::cerr << "Error: Invalid ROM access in switch_completed()" << std::endl;
        return;
    }
    
    Rom& rom = filtered_roms_list[selected_index];
    DB   db;
    for (auto& r : roms_list) {
        if (r.file == rom.file) {
            r = rom = db.save(rom.file, 0, rom.completed ? 0 : 1);
            break;
        }
    }
}

void Activities::set_pid(pid_t pid)
{
    // Safety check
    if (filtered_roms_list.empty() || selected_index >= static_cast<int>(filtered_roms_list.size())) {
        std::cerr << "Error: Invalid ROM access in set_pid()" << std::endl;
        return;
    }
    
    Rom& rom = filtered_roms_list[selected_index];
    DB   db;
    for (auto& r : roms_list) {
        if (r.file == rom.file) {
            r.pid = rom.pid = pid;
            break;
        }
    }
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
    for (const auto& rom : roms_list) {
        if (rom.system == current_system || system_index == 0) {
            if (filter_state == 0 || (filter_state == 1 && rom.pid != -1) ||
                (filter_state == 2 && rom.completed == 1) ||
                (filter_state == 3 && rom.completed == 0)) {
                filtered_roms_list.push_back(rom);
                total_time += rom.time;
            }
        }
    }
    list_size = filtered_roms_list.size();

    sort_roms();

    if (selected_index >= static_cast<int>(list_size))
        selected_index = list_size == 0 ? 0 : static_cast<int>(list_size - 1);
}

void Activities::sort_roms()
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

void Activities::game_list()
{
    Vec2 prevSize;

    gui.render_image(cfg.theme_path + "skin/title-bg.png", gui.Width / 2, FONT_MIDDLE_SIZE,
        gui.Width, 2 * FONT_MIDDLE_SIZE);

    // Display the current/total index at the bottom right
    if (list_size > 0) {
        std::string index_text = std::to_string(selected_index + 1) + "/" + std::to_string(list_size);
        // Calculate text width to position it properly from the right edge
        int text_width = index_text.length() * (FONT_TINY_SIZE * 0.7); // Approximate character width
        gui.render_text(index_text, gui.Width - text_width - 10, gui.Height - FONT_MINI_SIZE * 2.5 - 50, FONT_TINY_SIZE, cfg.unselect_color);
    }
    // Centered title
    std::string current_system_name = "";
    if (!systems.empty() && system_index < systems.size()) {
        current_system_name = systems[system_index];
    }
    gui.render_text(states_names[filter_state] + " " + current_system_name + " Games",
        gui.Width / 2 - 100, 0, FONT_MIDDLE_SIZE, cfg.unselect_color);

    // Total time to the right of the title
    gui.render_text("(" + utils::stringifyTime(total_time) + ")", gui.Width - 2 * FONT_MIDDLE_SIZE, 0.4 * FONT_MIDDLE_SIZE, FONT_TINY_SIZE, cfg.unselect_color, 0, true);
    size_t first = (list_size <= LIST_LINES)
                       ? 0
                       : std::max(0, static_cast<int>(selected_index) - LIST_LINES / 2);
    size_t last = std::min(first + LIST_LINES, filtered_roms_list.size());

    int y = 80;
    int x = 10;
    for (size_t j = first; j < last; ++j) {
        const Rom& rom = filtered_roms_list[j];
        SDL_Color  color = (static_cast<int>(j) == selected_index) ? cfg.selected_color : cfg.unselect_color;

        if (static_cast<int>(j) == selected_index) {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-f.png", x, y, 0, 0, IMG_NONE);
        } else {
            prevSize = gui.render_image(
                cfg.theme_path + "skin/list-item-1line-sort-bg-n.png", x, y, 0, 0, IMG_NONE);
        }

        // Green dot if game is running (pid != -1)
        if (rom.pid != -1) {
            int dot_radius = 8;
            int dot_x = x + 8;
            int dot_y = y + 8 + FONT_MIDDLE_SIZE / 2;
            gui.draw_green_dot(dot_x, dot_y, dot_radius);
        }

        // Display game name (accounting for the dot)
        int name_offset = rom.pid != -1 ? 2 * 8 + 6 : 15;
        if (static_cast<int>(j) == selected_index) {
            gui.render_scrollable_text(rom.name, x + name_offset, y + 2, prevSize.x - 5 - name_offset, FONT_MIDDLE_SIZE, color);
        } else {
            gui.render_text(rom.name, x + name_offset, y + 2, FONT_MIDDLE_SIZE, color, prevSize.x - 5 - name_offset);
        }

        gui.render_multicolor_text(
            {{"Time: ", cfg.unselect_color}, {rom.total_time, color},
                {"  Count: ", cfg.unselect_color}, {std::to_string(rom.count), color},
                {"  Last: ", cfg.unselect_color}, {rom.last, color}},
            x + 15, y + prevSize.y / 2 + 6, FONT_TINY_SIZE);

        y += prevSize.y + 8;
    }
    if (list_size && gui.Width == 1280 && selected_index < static_cast<int>(filtered_roms_list.size())) {
        gui.render_image(cfg.theme_path + "skin/ic-game-580.png", 1070, 370, 400, 580);
        gui.render_image(filtered_roms_list[selected_index].image, 1070, 370, 400, 0);
    }

    gui.render_image(cfg.theme_path + "skin/tips-bar-bg.png", gui.Width / 2, gui.Height - 20,
        gui.Width, FONT_MINI_SIZE * 2);
    gui.display_keybind("A", "Select", 25);
    gui.display_keybind("B", "Quit", 140);
    gui.display_keybind("X", "Details", 230);
    gui.display_keybind("Menu", "Global stats", 340);
    gui.display_keybind("L1", "R1", "Change system", gui.Width - 620);
    //gui.display_keybind("L2", "Manual", gui.Width - 500);
    gui.display_keybind("Select", "State filter", gui.Width - 350);
    gui.display_keybind("Start", "Sort by (" + sort_names[sort_by] + ")", gui.Width - 180);

    gui.render();

    size_t    prev_selected_index = selected_index;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        InputAction action = gui.map_input(e);
        // Make sure filtered_roms_list is not empty before accessing selected_index
        const bool has_rom = !filtered_roms_list.empty() && selected_index < static_cast<int>(filtered_roms_list.size());
        const Rom& rom = has_rom ? filtered_roms_list[selected_index] : *(Rom*)nullptr;
        switch (action) {
        case InputAction::Quit: is_running = false; break;
        case InputAction::Up:
            if (selected_index > 0)
                selected_index--;
            break;
        case InputAction::Down:
            if (selected_index < static_cast<int>(list_size) - 1)
                selected_index++;
            break;
        case InputAction::Left:
            selected_index = selected_index > 10 ? selected_index - 10 : 0;
            break;
        case InputAction::Right:
            selected_index = list_size > 10 && selected_index < static_cast<int>(list_size) - 10 ? selected_index + 10
                                                                               : static_cast<int>(list_size) - 1;
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
        // case InputAction::A:
        //     in_game_detail = true;
        //     gui.reset_scroll();
        //     break;
        case InputAction::A:
            if (has_rom) {
                gui.launch_game(rom.name, rom.system, rom.file);
                pid_t ret = gui.wait_game(rom.name);
                set_pid(ret);
                // If the game is exited or paused, request a refresh from the GUI return.
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
            break;
        case InputAction::Start:
            sort_by = sort_by == Sort::Last ? Sort::Name : static_cast<Sort>(sort_by + 1);
            sort_roms();
            break;
        case InputAction::Menu: overall_stats(); break;
        default: break;
        }
    }
    if (static_cast<int>(prev_selected_index) != selected_index)
        gui.reset_scroll();
}

void Activities::game_detail()
{
    // Safety check
    if (filtered_roms_list.empty() || selected_index >= static_cast<int>(filtered_roms_list.size())) {
        std::cerr << "Error: Invalid ROM access in game_detail()" << std::endl;
        in_game_detail = false;
        return;
    }
    
    Rom& rom = filtered_roms_list[selected_index];

    // Header: Game name
    gui.render_image(cfg.theme_path + "skin/title-bg.png", gui.Width / 2, FONT_MIDDLE_SIZE,
        gui.Width, 2 * FONT_MIDDLE_SIZE);
    gui.render_scrollable_text(
        rom.name, 10, 0, gui.Width - 2 * 10, FONT_MIDDLE_SIZE, cfg.unselect_color);

    // Left side: Rom image
    gui.render_image(cfg.theme_path + "skin/bg-menu-09.png", gui.Width / 4, gui.Height / 2 -10,
        gui.Width / 2 - 60, gui.Width / 2 - 60);
    if (!rom.image.empty())
        gui.render_image(
            rom.image, gui.Width / 4, gui.Height / 2 - 10, 0, gui.Width / 2 - 85, IMG_FIT | IMG_CENTER);
    else
        gui.render_image(cfg.theme_path + "skin/ic-keymap-n.png", gui.Width / 4, gui.Height / 2);

    // Right side: Game details
    std::vector<std::pair<std::string, std::string>> details = {
        {"Total Time: ", rom.total_time.empty() ? "N/A" : rom.total_time},
        {"Average Time: ", rom.average_time.empty() ? "N/A" : rom.average_time},
        {"Last played: ", rom.last.empty() ? "N/A" : rom.last},
        {"Last session: ", utils::stringifyTime(rom.lastsessiontime)},
        {"Play count: ", std::to_string(rom.count)},
        {"System: ", rom.system.empty() ? "N/A" : rom.system},
        {"Completed: ", rom.completed ? "Yes" : "No"}
    };
    gui.infos_window("Informations", FONT_TINY_SIZE, details, FONT_MINI_SIZE,
        3 * gui.Width / 4 - 10, gui.Height / 2, gui.Width / 2 - 50, gui.Height / 2);

    // Bottom: File path.
    gui.render_image(cfg.theme_path + "skin/list-item-2line-long-bg-n.png", gui.Width / 2,
        gui.Height - FONT_MINI_SIZE * 3.5, gui.Width - 2 * 10, FONT_MINI_SIZE * 2,
        IMG_FIT | IMG_CENTER);
    gui.render_text(
        "File:", 50, gui.Height - FONT_MINI_SIZE * 4.5 + 4, FONT_MINI_SIZE, cfg.selected_color);
    gui.render_text(
        rom.file, 100, gui.Height - FONT_MINI_SIZE * 4.5 + 4, FONT_MINI_SIZE, cfg.info_color);

    gui.render_image(cfg.theme_path + "skin/tips-bar-bg.png", gui.Width / 2, gui.Height - 20,
        gui.Width, FONT_MINI_SIZE * 2);
    gui.display_keybind("A", "Start", 25);
    gui.display_keybind("B", "Return", 140);
    gui.display_keybind("X", "Back to list", 250);
    gui.display_keybind("Menu", "Remove", gui.Width / 2 - 150);
    //gui.display_keybind("L2", "Manual", gui.Width / 2 - 80);
    gui.display_keybind("Select", rom.completed ? "Uncomplete" : "Complete", gui.Width / 2);
    
    // Display navigation arrows on the screen sides if navigation is possible
    if (filtered_roms_list.size() > 1) {
        // Left arrow (newer elements)
        if (selected_index > 0) {
            gui.render_image(cfg.theme_path + "skin/ic-right-arrow-n.png", gui.Width - 10, gui.Height / 2, 40, 40);
        }
        // Right arrow (older elements)
        if (selected_index < static_cast<int>(filtered_roms_list.size()) - 1) {
            gui.render_image(cfg.theme_path + "skin/ic-left-arrow-n.png", 10, gui.Height / 2, 40, 40);
        }

        
        int nav_x = gui.Width - 320;
        gui.render_image(cfg.theme_path + "skin/ic-left-arrow-n.png", nav_x, gui.Height - 20, 30, 30);
        gui.render_image(cfg.theme_path + "skin/ic-right-arrow-n.png", nav_x + 35, gui.Height - 20, 30, 30);
        gui.render_text("Navigate", nav_x + 50, gui.Height - 32, FONT_MINI_SIZE, cfg.info_color);
   
    }
    
    if (!rom.video.empty())
        gui.display_keybind("Y", "Video", gui.Width - 240);
    if (!rom.manual.empty())
        gui.display_keybind("X", "Manual", gui.Width - 125);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (gui.map_input(e)) {
        case InputAction::Quit: is_running = false; break;
        //case InputAction::B: in_game_detail = false; break;
        case InputAction::B: is_running = false; break;
        case InputAction::Left:
            if (selected_index < static_cast<int>(filtered_roms_list.size()) - 1) {
                selected_index++;
                gui.reset_scroll();
            }
            break;
        case InputAction::Right:
            if (selected_index > 0) {
                selected_index--;
                gui.reset_scroll();
            }
            break;
        case InputAction::A:
            gui.launch_game(rom.name, rom.system, rom.file);
            {
                pid_t ret = gui.wait_game(rom.name);
                set_pid(ret);
                // If the game is exited or paused, request a refresh from the GUI return.
                if (ret == -1 || ret == 0)
                    need_refresh = true;
            }
            break;
        case InputAction::ZL:
            if (!rom.video.empty())
                gui.launch_external(std::string(VIDEO_PLAYER) + " \"" + rom.video + "\"");
            break;
        case InputAction::X:
            // Allow switching to list mode even in ROM-specific mode
            // since now all ROMs are loaded
            in_game_detail = false;
            break;
        case InputAction::ZR:
            if (!rom.manual.empty())
                gui.launch_external(std::string(MANUAL_READER) + " \"" + rom.manual + "\"");
            break;
        case InputAction::Select: switch_completed(); break;
        case InputAction::Menu:
            if (gui.confirmation_popup("Remove game from DB?", FONT_MIDDLE_SIZE)) {
                DB db;
                db.remove(rom.file);
                roms_list.erase(std::remove_if(roms_list.begin(), roms_list.end(),
                                    [&rom](const Rom& r) { return r.file == rom.file; }),
                    roms_list.end());
                filter_roms();
                // If we're in ROM-specific mode and delete the ROM, quit the application
                if (no_list) {
                    is_running = false;
                } else {
                    in_game_detail = false;
                }
            }
            break;
        default: break;
        }
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
    gui.render_text("No datas available.", gui.Width / 2, gui.Height / 2, FONT_BIG_SIZE,
        cfg.title_color, 0, true);
    gui.render();
}

void Activities::run()
{
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
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - refresh_timer_start);
                
                if (elapsed.count() >= 1000) {
                    // 1 second has elapsed, proceed with refresh
                    refresh_timer_active = false;
                    
                    // Save the current selected rom file (if any)
                    std::string selected_file;
                    if (!filtered_roms_list.empty() && selected_index < static_cast<int>(filtered_roms_list.size()))
                        selected_file = filtered_roms_list[selected_index].file;

                    DB db;
                    
                    if (no_list) {
                        // In ROM-specific mode, only reload the current ROM
                        if (!filtered_roms_list.empty()) {
                            std::string current_rom_file = filtered_roms_list[0].file;
                            Rom updated_rom = db.load(current_rom_file);
                            if (!updated_rom.file.empty()) {
                                roms_list[0] = updated_rom;
                                filtered_roms_list[0] = updated_rom;
                            }
                        }
                    } else {
                        // In full list mode, reload the entire database
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
                        filter_roms();
                    }

                    // Restore selection to the same rom if possible
                    if (!selected_file.empty()) {
                        for (size_t i = 0; i < filtered_roms_list.size(); ++i) {
                            if (filtered_roms_list[i].file == selected_file) {
                                selected_index = i;
                                break;
                            }
                        }
                    }
                    need_refresh = false;
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
