#include "Activities.h"
#include "utils.h"
#include <iostream>
#include <set>
#include <fstream>

Activities::Activities()
    : cfg(Config::getInstance())
    , gui(GUI::getInstance())
    , db(DB::getInstance())
{
    gui.init();
    is_running = true;
    refresh_db();
}

Activities::~Activities()
{
}

void Activities::filter_roms()
{
    // Safety check to avoid out-of-bounds access
    std::string current_system = "All";
    if (!systems.empty() && system_index < systems.size())
        current_system = systems[system_index];

    filtered_roms_list.clear();
    total_time = 0;
    for (auto it = roms_list.begin(); it != roms_list.end(); it++) {
        if (it->system == current_system || system_index == 0) {
            // != Unmatch mean All and Match;  < Match mean All and Unmatch
            if (((filters_states.running != FilterState::Unmatch && it->pid != -1) ||
                    (filters_states.running < FilterState::Match && it->pid == -1)) &&
                ((filters_states.favorites != FilterState::Unmatch && it->favorite == 1) ||
                    (filters_states.favorites < FilterState::Match && it->favorite == 0)) &&
                ((filters_states.completed != FilterState::Unmatch && it->completed == 1) ||
                    (filters_states.completed < FilterState::Match && it->completed == 0))) {
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
    bool rev = reverse_sort;
    switch (sort_by) {
    case Sort::Name:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [rev](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                bool ret = a->name < b->name;
                return rev ? !ret : ret;
            });
        break;
    case Sort::Time:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [rev](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                bool ret = a->total_time > b->total_time;
                return rev ? !ret : ret;
            });
        break;
    case Sort::Count:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [rev](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                bool ret = a->count > b->count;
                return rev ? !ret : ret;
            });
        break;
    case Sort::Last:
        std::sort(filtered_roms_list.begin(), filtered_roms_list.end(),
            [rev](std::vector<Rom>::iterator a, std::vector<Rom>::iterator b) {
                bool ret = a->last > b->last;
                return rev ? !ret : ret;
            });
        break;
    }
    gui.reset_scroll();
}

MenuResult Activities::switch_filter(const std::string& label, int& state)
{
    std::string str = gui.string_selector("Show:",
        {std::string(state == FilterState::Match ? "*" : "") + "Only " + label,
            std::string(state == FilterState::Unmatch ? "*" : "") + "Only not " + label,
            std::string(state == FilterState::All ? "*" : "") + "Both"},
        gui.Width / 2, true);
    if (str == "Only " + label) {
        state = FilterState::Match;
    } else if (str == "Only not " + label) {
        state = FilterState::Unmatch;
    } else if (str == "Both") {
        state = FilterState::All;
    } else {
        return MenuResult::ExitCurrent;
    }
    filter_roms();
    return MenuResult::ExitAll;
}

void Activities::game_menu(std::vector<Rom>::iterator rom)
{
    std::vector<std::pair<std::string, MenuAction>> menu_items;

    if (filtered_roms_list.empty())
        return;
    menu_items = {{rom->pid == -1 ? "Start" : "Resume",
                      [this, rom]() -> MenuResult {
                          rom->start();
                          handle_game_return(rom->wait());
                          return MenuResult::ExitAll;
                      }},
        {rom->completed ? "Uncomplete" : "Complete",
            [this, &rom]() -> MenuResult {
                rom->completed = rom->completed ? 0 : 1;
                rom->save();
                filter_roms();
                return MenuResult::ExitAll;
            }},
        {rom->favorite ? "UnFavorite" : "Favorite",
            [&rom]() -> MenuResult {
                rom->favorite = rom->favorite ? 0 : 1;
                rom->save();
                return MenuResult::ExitAll;
            }},
        {"Remove DB entry",
            [this, &rom]() -> MenuResult {
                if (gui.confirmation_popup("Remove game from DB?", FONT_MIDDLE_SIZE)) {
                    db.remove(rom->file);
                    roms_list.erase(std::remove_if(roms_list.begin(), roms_list.end(),
                                        [&rom](const Rom& r) { return r.file == rom->file; }),
                        roms_list.end());
                    filter_roms();
                }
                return MenuResult::ExitAll;
            }},
        {"Change Launcher", [this, &rom]() -> MenuResult {
             std::vector<std::string> launchers = utils::get_launchers(rom->system);
             std::string              str =
                 gui.string_selector("Select new launcher:", launchers, gui.Width / 2, true);
             if (!str.empty()) {
                 utils::set_launcher(rom->system, rom->name, str);
                 rom->launcher = str;
                 return MenuResult::ExitAll;
             }
             return MenuResult::Continue;
         }}};

    if (rom->pid != -1)
        menu_items.insert(menu_items.begin() + 1, {"Stop", [this, rom]() -> MenuResult {
                                                       rom->stop();
                                                       filter_roms();
                                                       return MenuResult::ExitAll;
                                                   }});
    gui.save_background_texture();
    gui.menu("Game menu", menu_items);
    gui.delete_background_texture();
}

MenuResult Activities::sort_menu()
{
    std::vector<std::pair<std::string, MenuAction>> menu_items;

    menu_items = {{"Sort by...",
                      [this]() -> MenuResult {
                          std::string str = gui.string_selector("Sort by...",
                              {std::string(sort_by == Sort::Last ? "*" : "") + "Last",
                                  std::string(sort_by == Sort::Name ? "*" : "") + "Name",
                                  std::string(sort_by == Sort::Count ? "*" : "") + "Count",
                                  std::string(sort_by == Sort::Time ? "*" : "") + "Time"},
                              gui.Width / 2, true);
                          if (str == "Last") {
                              sort_by = Sort::Last;
                          } else if (str == "Name") {
                              sort_by = Sort::Name;
                          } else if (str == "Count") {
                              sort_by = Sort::Count;
                          } else if (str == "Time") {
                              sort_by = Sort::Time;
                          } else {
                              return MenuResult::Continue;
                          }
                          sort_roms();
                          return MenuResult::ExitAll;
                      }},
        {"Reverse Sort", [this]() -> MenuResult {
             reverse_sort = !reverse_sort;
             sort_roms();
             return MenuResult::ExitAll;
         }}};

    return gui.menu("Sorting...", menu_items);
}

MenuResult Activities::filters_menu()
{
    std::vector<std::pair<std::string, MenuAction>> menu_items;

    menu_items = {{"System",
                      [this]() -> MenuResult {
                          std::string str = gui.string_selector(
                              "Which system to show?", systems, gui.Width / 2, true);
                          if (str.empty()) {
                              return MenuResult::Continue;
                          } else if (str == "All") {
                              system_index = 0;
                          } else {
                              system_index = std::distance(
                                  systems.begin(), find(systems.begin(), systems.end(), str));
                          }
                          filter_roms();
                          return MenuResult::ExitAll;
                      }},
        {"Running",
            [this]() -> MenuResult { return switch_filter("running", filters_states.running); }},
        {"Favorites",
            [this]() -> MenuResult {
                return switch_filter("favorites", filters_states.favorites);
            }},
        {"Completed", [this]() -> MenuResult {
             return switch_filter("completed", filters_states.completed);
         }}};

    return gui.menu("Filters...", menu_items);
}

void Activities::global_menu()
{
    std::vector<std::pair<std::string, MenuAction>> menu_items;

    menu_items = {{"Filters", [this]() -> MenuResult { return filters_menu(); }},
        {"Sort", [this]() -> MenuResult { return sort_menu(); }},
        {"Add Game",
            [this]() -> MenuResult {
                std::string str = gui.file_selector(fs::path("/mnt/SDCARD/Roms"), true);
                if (str.empty())
                    return MenuResult::Continue;
                refresh_db(str); // refresh_db will save the rom as it will not find it in db
                return MenuResult::ExitAll;
            }},
        {"Global Stats",
            [this]() -> MenuResult {
                overall_stats();
                return MenuResult::Continue;
            }},
        {"Exit", [this]() -> MenuResult {
             is_running = false;
             return MenuResult::ExitAll;
         }}};

    gui.save_background_texture();
    gui.menu("", menu_items);
    gui.delete_background_texture();
}

void Activities::handle_game_return(int wait_status)
{
    switch (wait_status) {
    case 1:
        sort_by = Sort::Last;
        filters_states = {FilterState::Match, FilterState::All, FilterState::All};
        break;
    case 2:
        sort_by = Sort::Last;
        filters_states = {FilterState::All, FilterState::Match, FilterState::All};
        break;
    case 0:
    case 3:
        sort_by = Sort::Last;
        filters_states = {FilterState::All};
        break;
    }
    filter_roms();
}

/**
 * @brief Starts an external application by executing a shell command.
 *
 * @details This function is designed to launch an external process, such as tool. It temporarily
 * disables SDL joystick button events to prevent unwanted input from being processed
 * while the external command is running. The command is executed using `system()`,
 * which is a blocking call, meaning the function will not return until the external
 * process has completed. After the command finishes, it re-enables the joystick events
 * and waits for an event to clear the event queue, ensuring the GUI is ready to
 * receive input again.
 *
 * @param command A constant reference to a string containing the shell command to
 * be executed.
 */
void Activities::start_external(const std::string& command)
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
    // TODO once filters reworked: avoid "All - All Games" when system is set to All
    gui.render_text(current_system_name + " games:", gui.Width / 2 - 100, 0, FONT_MIDDLE_SIZE,
        cfg.unselect_color);

    // Total time to the right of the title
    gui.render_text("(" + utils::stringifyTime(total_time) + ")", gui.Width - 2 * FONT_MIDDLE_SIZE,
        0.4 * FONT_MIDDLE_SIZE, FONT_TINY_SIZE, cfg.unselect_color, 0, true);
    size_t first = (list_size <= LIST_LINES)
                       ? 0
                       : std::max(0, static_cast<int>(selected_index) - LIST_LINES / 2);
    size_t last = std::min(first + LIST_LINES, filtered_roms_list.size());

    int y = 80;
    int x = 10;

    for (size_t j = first; j < last; j++) {
        std::vector<Rom>::iterator rom = filtered_roms_list[j];
        SDL_Color color = (j == selected_index) ? cfg.selected_color : cfg.unselect_color;

        prevSize = gui.render_image(cfg.theme_path + "skin/list-item-1line-sort-bg-" +
                                        (j == selected_index ? "f" : "n") + ".png",
            x, y, 0, 0, IMG_NONE);

        int offset = 5;

        if (rom->completed)
            offset += gui.render_image(std::string(APP_DIR) + "/.assets/green_check.svg",
                             x + offset, y + FONT_MIDDLE_SIZE / 2, 16, 16, IMG_NONE)
                          .x;
        if (rom->favorite)
            offset += gui.render_image(cfg.theme_path + "skin/icon-star.png", x + offset,
                             y + FONT_MIDDLE_SIZE / 2, 16, 16, IMG_NONE)
                          .x;
        if (rom->pid != -1)
            offset += gui.render_image(std::string(APP_DIR) + "/.assets/green_dot.svg", x + offset,
                             y + FONT_MIDDLE_SIZE / 2, 16, 16, IMG_NONE)
                          .x;
        offset = std::max(15, offset);
        // Display game name (accounting for the icons)
        if (j == selected_index) {
            gui.render_scrollable_text(
                rom->name, x + offset, y + 2, prevSize.x - 5 - offset, FONT_MIDDLE_SIZE, color);
        } else {
            gui.render_text(
                rom->name, x + offset, y + 2, FONT_MIDDLE_SIZE, color, prevSize.x - 5 - offset);
        }

        gui.render_multicolor_text(
            {{"Time: ", cfg.unselect_color}, {rom->total_time, color},
                {"  Count: ", cfg.unselect_color}, {std::to_string(rom->count), color},
                {"  Last: ", cfg.unselect_color}, {rom->last, color}},
            x + 15, y + prevSize.y / 2 + 6, FONT_TINY_SIZE);

        y += prevSize.y + 8;
    }
    if (list_size && gui.Width == 1280 && selected_index < filtered_roms_list.size()) {
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
    gui.display_keybind("Select", "Filters Menu", gui.Width - 350);
    gui.display_keybind("Start", "Game Menu", gui.Width - 180);

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
        const bool has_rom =
            !filtered_roms_list.empty() && selected_index < filtered_roms_list.size();
        std::vector<Rom>::iterator rom =
            has_rom ? filtered_roms_list[selected_index] : roms_list.end();
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
            if (selected_index < list_size - 1)
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

            do {
                system_index = (system_index == 0) ? systems.size() - 1 : system_index - 1;
                filter_roms();
            } while (filtered_roms_list.empty() && system_index > 0);
            upHolding = downHolding = false;
            break;
        case InputAction::R1:
            do {
                system_index = (system_index + 1) % systems.size();
                filter_roms();
            } while (filtered_roms_list.empty() && system_index > 0);
            upHolding = downHolding = false;
            break;
        case InputAction::A:
        case InputAction::Y: {
            if (has_rom) {
                upHolding = downHolding = false;
                rom->start();
                handle_game_return(rom->wait());
            }
            break;
        }
        case InputAction::B: is_running = false; break;
        case InputAction::X:
            upHolding = downHolding = false; // stop repeat on jump navigation
            in_game_detail = true;
            gui.reset_scroll();
            break;
        case InputAction::ZR:
            if (has_rom && !rom->manual.empty()) {
                upHolding = downHolding = false;
                // game_runner.start_external(std::string(MANUAL_READER) + " \"" + rom->manual + "\"");
            }
            break;
        //
        // case InputAction::Select:
        //     filter_state = (filter_state + 1) % 4;
        //     filter_roms();
        //     upHolding = downHolding = false;
        //     break;
        // case InputAction::Start:
        //     sort_by = sort_by == Sort::Last ? Sort::Name : static_cast<Sort>(sort_by + 1);
        //     sort_roms();
        //     upHolding = downHolding = false;
        //     break;
        case InputAction::Menu:
            upHolding = downHolding = false;
            global_menu();
            break;
        case InputAction::Select: upHolding = downHolding = false; break;
        case InputAction::Start:
            upHolding = downHolding = false;
            game_menu(rom);
            break;
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
    if (filtered_roms_list.empty() || selected_index >= filtered_roms_list.size()) {
        std::cerr << "Error: Invalid ROM access in game_detail()" << std::endl;
        gui.save_background_texture();
        gui.message_popup(3000, {{"No game to display", 28, cfg.title_color},
                                    {"Reseting filters.", 24, cfg.selected_color}});
        gui.delete_background_texture();
        filters_states = {FilterState::All, FilterState::All, FilterState::All};
        system_index = 0;
        filter_roms();
        if (filtered_roms_list.empty()) {
            in_game_detail = false;
            return;
        }
    }

    std::vector<Rom>::iterator rom = filtered_roms_list[selected_index];

    // Header: Game name
    gui.render_image(cfg.theme_path + "skin/title-bg.png", gui.Width / 2, FONT_MIDDLE_SIZE,
        gui.Width, 2 * FONT_MIDDLE_SIZE);
    int offset = 10;
    if (rom->pid != -1)
        offset += gui.render_image(std::string(APP_DIR) + "/.assets/green_dot.svg",
                         offset + FONT_MIDDLE_SIZE / 2, offset + FONT_MIDDLE_SIZE / 2,
                         FONT_MIDDLE_SIZE * 1.5, FONT_MIDDLE_SIZE * 1.5, IMG_CENTER)
                      .x +
                  offset;

    gui.render_scrollable_text(
        rom->name, offset, 0, gui.Width - 2 * 10, FONT_MIDDLE_SIZE, cfg.unselect_color);

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
        size_t    displayCount = std::min(n, static_cast<size_t>(maxDots));
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
        {"Completed: ", rom->completed ? "Yes" : "No"}, {"Launcher: ", rom->launcher}};
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
    gui.display_keybind("Select", "Filters Menu", gui.Width - 350);
    gui.display_keybind("Start", "Game Menu", gui.Width - 180);
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

        int nav_x = gui.Width / 2;
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
        case InputAction::Quit:
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
        case InputAction::Y:
            // Y runs the game now (same as A)
            leftHolding = rightHolding = false;
            rom->start();
            handle_game_return(rom->wait());
            break;
        case InputAction::ZL:
            if (!rom->video.empty()) {
                leftHolding = rightHolding = false;
                // game_runner.start_external(std::string(VIDEO_PLAYER) + " \"" + rom->video + "\"");
            }
            break;
        case InputAction::X:
            in_game_detail = false;
            leftHolding = rightHolding = false;
            break;
        case InputAction::ZR:
            if (!rom->manual.empty()) {
                leftHolding = rightHolding = false;
                // game_runner.start_external(std::string(MANUAL_READER) + " \"" + rom->manual + "\"");
            }
            break;
        case InputAction::Menu:
            leftHolding = rightHolding = false;
            global_menu();
            break;
        case InputAction::Select: leftHolding = rightHolding = false; break;
        case InputAction::Start:
            leftHolding = rightHolding = false;
            game_menu(rom);
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
                if (leftHolding && selected_index < filtered_roms_list.size() - 1) {
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
    while (running && is_running) {

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

        gui.render_background();
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
    } while (++it != roms_list.end());
    return nullptr;
}

void Activities::refresh_db(std::string selected_rom_file)
{
    // Save the current selected rom file (if any)
    if (selected_rom_file.empty()) {
        filter_roms(); // refresh filtered to avoid iterators invalids if a rom was added to
                       // roms_list.
        if (!filtered_roms_list.empty() && selected_index < filtered_roms_list.size())
            selected_rom_file = filtered_roms_list[selected_index]->file;
    } else {
        selected_rom_file = utils::shorten_file_path(selected_rom_file);

        if (!get_rom(selected_rom_file)) {
            std::cout << "ROM not found in database, creating new entry for: " << selected_rom_file
                      << std::endl;
            Rom rom(selected_rom_file);
            rom.save();
        }
    }

    roms_list = Rom::getAll(roms_list);

    std::set<std::string> unique_systems;
    for (const auto& rom : roms_list) {
        unique_systems.insert(rom.system);
    }
    systems.clear();
    systems.push_back("All");
    systems.insert(systems.end(), unique_systems.begin(), unique_systems.end());
    filter_roms();
    // Restore selection to the same rom if possible
    for (size_t i = 0; i < filtered_roms_list.size(); i++) {
        if (filtered_roms_list[i]->file == selected_rom_file) {
            selected_index = i;
            break;
        }
    }
    if (selected_index >= filtered_roms_list.size())
        selected_index = 0;

    std::cout << "db_refreshed !! Selected_index: " << selected_index << std::endl;
    // Debug: display loaded values
    if (selected_index < filtered_roms_list.size()) {
        std::vector<Rom>::iterator loaded_rom = filtered_roms_list[selected_index];
        std::cout << "Final ROM data:" << std::endl;
        std::cout << "  Name: " << loaded_rom->name << std::endl;
        std::cout << "  File: " << loaded_rom->file << std::endl;
        std::cout << "  Count: " << loaded_rom->count << std::endl;
        std::cout << "  Time: " << loaded_rom->time << std::endl;
        std::cout << "  Total time: '" << loaded_rom->total_time << "'" << std::endl;
        std::cout << "  Average time: '" << loaded_rom->average_time << "'" << std::endl;
        std::cout << "  System: '" << loaded_rom->system << "'" << std::endl;
        std::cout << "  Launcher: '" << loaded_rom->launcher << "'" << std::endl;
        std::cout << "  Last: '" << loaded_rom->last << "'" << std::endl;
        std::cout << "  Selected index: " << selected_index << std::endl;
        std::cout << "  Total ROMs loaded: " << roms_list.size() << std::endl;
        std::cout << "  Filtered ROMs: " << filtered_roms_list.size() << std::endl;
    }
}

static const char gui_help[] = {
    "activities GUI usage:\n"
    "\tactivities gui [option...]*\n"
    "Options:\n"
    "  -h\tDisplay help text\n"
    "  -D\tStart the gui in details mode instead of list\n"
    "  -s\tSet the initial sort method. Default: (name,time,count,\e[0mlast\e[1m)\n"
    "  -r\tReverse the initial sort order. "
    "  -S\tSet the initial system to filter. (\e[1mall\e[0m,gba,psp,fc...)\n"
    "  -c\tSet the initial completed filter (\e[1mall\e[1m,on,off)\n"
    "  -f\tSet the initial romFile to display. default: first of the filtered and sorted list. "
    "\n"};

extern char* optarg;
extern int   optind;

static const char options[] = "hDRs:rS:c:f:";
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
        case 'D': in_game_detail = true; break;
        case 'R': auto_resume_enabled = false; break;
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
        case 'r': reverse_sort = true; break;
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
        // case 'c':
        //     if (optarg == NULL)
        //         break;
        //     tmp_str = std::string(optarg);
        //     if (tmp_str == "all") {
        //         filter_state = 0;
        //     } else if (tmp_str == "on") {
        //         filter_state = 2;
        //     } else if (tmp_str == "off") {
        //         filter_state = 3;
        //     }
        //     break;
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

void Activities::auto_resume()
{
    std::ifstream file("/mnt/SDCARD/Apps/Activities/data/autostarts.txt");
    if (file.fail())
        return;
    std::cout << "\t Auto-resume games..." << std::endl;
    std::string       romFile;
    std::vector<Rom*> ordered_roms;
    while (std::getline(file, romFile)) {
        // Check if the ROM file exists before attempting to start it

        if (fs::exists(romFile)) {
            Rom* rom_ptr = get_rom(romFile);
            
            if (!rom_ptr) {
                std::cout << "ROM " << romFile << " not found in DB, creating new entry."
                          << std::endl;
                Rom rom(romFile);
                rom.save(); // Save the new ROM entry
                roms_list.push_back(rom);
                rom_ptr = &roms_list.back(); 
            } else {
                std::cout << "Found rom: " << rom_ptr->name << std::endl;
            }
            ordered_roms.push_back(rom_ptr);
        } else {
            std::cerr << "Autostart ROM file not found: " << romFile << std::endl;
        }
    }
    std::sort(
        ordered_roms.begin(), ordered_roms.end(), [](Rom* a, Rom* b) { return a->last < b->last; });

    // sort games by last session date and stay on most recent one.
    for (auto rom_it = ordered_roms.begin(); rom_it < ordered_roms.end() - 1; rom_it++) {
        (*rom_it)->start();
        // Wait for the game to finish loading and update its PID
        sleep(2);
        (*rom_it)->pid = (*rom_it)->suspend();
    }
    // Keep latest played game active.
    ordered_roms.back()->start();
    handle_game_return(ordered_roms.back()->wait());
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
    if (auto_resume_enabled)
        auto_resume();

    while (is_running) {
        if (db.is_refresh_needed())
            refresh_db();
        // Safety check to avoid out-of-bounds access
        std::string current_system = "All";
        if (!systems.empty() && system_index < systems.size())
            current_system = systems[system_index];
        gui.render_background(current_system);
        if (in_game_detail)
            game_detail();
        else
            game_list();
        SDL_Delay(16); // ~60 FPS
    }
}
