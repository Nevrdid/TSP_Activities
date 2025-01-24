#include "GUI.h"

GUI::GUI(const Config& cfg)
    : cfg(cfg)
{
}

int GUI::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    joystick = SDL_JoystickOpen(0);
    if (joystick == NULL)
        std::cerr << "Couldn't open joystick 0: " << SDL_GetError() << std::endl;
    else
        std::cout << "Joystick 0 opened successfully." << std::endl;

    SDL_JoystickEventState(SDL_ENABLE);

    window = SDL_CreateWindow("Game Timer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg.width, cfg.height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return -1;
    }

    return 0;
}

GUI::~GUI(){}

void GUI::clean()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    for (auto& texture : image_cache)
        if (texture.second.texture)
            SDL_DestroyTexture(texture.second.texture);
    image_cache.clear();

    for (auto& entry : cached_text)
        if (entry.texture)
            SDL_DestroyTexture(entry.texture);

    cached_text.clear();
    SDL_JoystickClose(joystick);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

InputAction GUI::map_input(const SDL_Event& e)
{
    if (e.type == SDL_JOYHATMOTION) {
        switch (e.jhat.value) {
        case SDL_HAT_UP: return InputAction::Up;
        case SDL_HAT_DOWN: return InputAction::Down;
        case SDL_HAT_LEFT: return InputAction::Left;
        case SDL_HAT_RIGHT: return InputAction::Right;
        }
    } else if (e.type == SDL_JOYBUTTONDOWN) {
        switch (e.jbutton.button) {
        case 0: return InputAction::B;
        case 1: return InputAction::A;
        case 2: return InputAction::Y;
        case 3: return InputAction::X;
        case 4: return InputAction::L1;
        case 5: return InputAction::R1;
        case 6: return InputAction::Select;
        case 7: return InputAction::Start;
        case 8: return InputAction::Menu;
        }
#if defined(USE_KEYBOARD)
    } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_k: return InputAction::Up;
        case SDLK_j: return InputAction::Down;
        case SDLK_h: return InputAction::Left;
        case SDLK_l: return InputAction::Right;
        case SDLK_RETURN: return InputAction::A;
        case SDLK_ESCAPE: return InputAction::B;
        case SDLK_s: return InputAction::Start;
        case SDLK_c: return InputAction::Select;
        case SDLK_v: return InputAction::Y;
        case SDLK_m: return InputAction::X;
        case SDLK_f: return InputAction::L1;
        case SDLK_g: return InputAction::R1;
        case SDLK_DELETE: return InputAction::Menu;
        }
#endif
    }

    if (e.type == SDL_QUIT) {
        return InputAction::Quit;
    }

    return InputAction::None;
}

void GUI::render()
{
    SDL_RenderPresent(renderer);
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

Vec2 GUI::render_image(
    const std::string& image_path, int x, int y, int w, int h, bool no_overflow, bool center)
{
    if (image_path.empty())
        return {0, 0};
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

    int      x0 = center ? x - width / 2 : x;
    int      y0 = center ? y - height / 2 : y;
    SDL_Rect dest_rect = {x0, y0, width, height};
    SDL_RenderCopy(renderer, cached_texture.texture, nullptr, &dest_rect);
    return {width, height};
}

CachedText& GUI::getCachedText(const std::string& text, TTF_Font* font, SDL_Color color)
{
    for (auto& cached : cached_text) {
        if (cached.text == text && cached.r == color.r && cached.g == color.g &&
            cached.b == color.b && cached.a == color.a)
            return cached;
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

    CachedText cached = {texture, surface->w, surface->h, text, color.r, color.g, color.b, color.a};
    cached_text.push_back(cached);

    SDL_FreeSurface(surface);
    return cached_text.back();
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

void GUI::render_text(
    const std::string& text, int x, int y, TTF_Font* font, SDL_Color color, int width, bool center)
{
    CachedText& cached = getCachedText(text, font, color);

    if (!cached.texture) {
        std::cerr << "Failed to retrieve or cache text texture." << std::endl;
        return;
    }

    if (center) {
        x -= cached.width / 2;
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
    CachedText&   cached_texture = getCachedText(text, font, color);

    if (scroll_reset) {
        last_update = SDL_GetTicks();
        scroll_reset = false;
        offset = 0;
    }

    if (cached_texture.width <= width) {
        render_text(text, x, y, font, color, width);
        return;
    }

    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_update > (offset > 0 ? 16 : 2000)) { // Adjust delays
        last_update = current_time;
        offset += 3; // Scrolling speed
        if (offset > cached_texture.width - width)
            scroll_reset = true;
    }

    SDL_Rect clip_rect = {offset, 0, width, TTF_FontHeight(font)};
    SDL_Rect render_rect = {x, y, width, TTF_FontHeight(font)};

    SDL_RenderCopy(renderer, cached_texture.texture, &clip_rect, &render_rect);
}

void GUI::reset_scroll()
{
    scroll_reset = true;
}
void GUI::render_background(const std::string& system)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    auto render_bg_image = [this](const std::string& path) {
        render_image(path, cfg.width / 2, cfg.height / 2, cfg.width, cfg.height);
    };

    std::string bg = "";
    if (system != "") {
        bg = "/mnt/SDCARD/Backgrounds/" + cfg.backgrounds_theme + "/" + system + ".png";
        if (!std::filesystem::exists(bg))
            bg = "";
    }
    if (bg == "") {
        bg = cfg.theme_path + "skin/bg.png";
        if (!std::filesystem::exists(bg)) {
            return;
        }
    }
    render_bg_image(bg);
}

void GUI::display_keybind(const std::string& btn, const std::string& text, int x, int y,
    TTF_Font* font, const SDL_Color color)
{
    int prevX = render_image(cfg.theme_path + "skin/" + buttons_icons[btn], x, y, 30, 30).x;
    render_text(text, x + prevX / 2, y - 15, font, color);
};
void GUI::display_keybind(const std::string& btn1, const std::string& btn2, const std::string& text,
    int x, int y, TTF_Font* font, SDL_Color color)
{
    int prevX = render_image(cfg.theme_path + "skin/" + buttons_icons[btn1], x, y, 30, 30).x;
    prevX = render_image(cfg.theme_path + "skin/" + buttons_icons[btn2], x + prevX, y, 30, 30).x;
    render_text(text, x + 3 * prevX / 2, y - 15, font, color);
};

void GUI::load_background_texture()
{
    if (background_texture){
        SDL_RenderCopy(renderer, background_texture, nullptr, nullptr);
        return;
  }

    SDL_Surface* screen_surface =
        SDL_CreateRGBSurfaceWithFormat(0, cfg.width, cfg.height, 32, SDL_PIXELFORMAT_RGBA8888);

    if (!screen_surface) {
        std::cerr << "Failed to create screen surface: " << SDL_GetError() << std::endl;
        return ;
    }

    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, screen_surface->pixels,
            screen_surface->pitch) != 0) {
        std::cerr << "Failed to read pixels: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(screen_surface);
        return ;
    }

    background_texture = SDL_CreateTextureFromSurface(renderer, screen_surface);
    SDL_FreeSurface(screen_surface); // Free the surface since it's no longer needed

    if (!background_texture) {
        std::cerr << "Failed to create background texture: " << SDL_GetError() << std::endl;
        return ;
    }
    SDL_RenderCopy(renderer, background_texture, nullptr, nullptr);
}

void GUI::unload_background_texture()
{

    if (background_texture) {
        SDL_DestroyTexture(background_texture);
        background_texture = nullptr;
    }
}

bool GUI::confirmation_popup(const std::string& message, TTF_Font* font)
{
    bool confirmed = false;
    bool running = true;
    Vec2 prevSize;
    while (running) {
        load_background_texture();
        render_image(cfg.theme_path + "skin/float-win-mask.png", cfg.width / 2, cfg.height / 2);
        render_image(
            cfg.theme_path + "skin/pop-bg.png", cfg.width / 2, cfg.height / 2, 0, 0, false, true);
        render_text(message, cfg.width / 2, cfg.height / 3, font, cfg.title_color, 0, true);

        prevSize = render_image(cfg.theme_path + "skin/btn-bg-" + (confirmed ? "f" : "n") + ".png",
            cfg.width / 3, 2 * cfg.height / 3);
        render_text("Yes", cfg.width / 3, 2 * cfg.height / 3 - prevSize.y / 2, font,
            confirmed ? cfg.selected_color : cfg.unselect_color, 0, true);
        prevSize = render_image(cfg.theme_path + "skin/btn-bg-" + (confirmed ? "n" : "f") + ".png",
            2 * cfg.width / 3, 2 * cfg.height / 3);
        render_text("No", 2 * cfg.width / 3, 2 * cfg.height / 3 - prevSize.y / 2, font,
            confirmed ? cfg.unselect_color : cfg.selected_color, 0, true);
        render();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (map_input(e)) {
            case InputAction::Left: confirmed = true; break;
            case InputAction::Right: confirmed = false; break;
            case InputAction::B: running = false; break;
            case InputAction::A: running = false; break;
            case InputAction::Quit: running = false; break;
            default: break;
            }
        }
    }
    unload_background_texture();
    return confirmed;
}
