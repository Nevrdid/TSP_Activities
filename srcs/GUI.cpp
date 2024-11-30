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

GUI::~GUI()
{

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

Vec2 GUI::render_image(const std::string& image_path, int x, int y, int w, int h, bool no_overflow)
{
    if (image_path.empty())
        return {0,0};
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
    return {width, height};
}

CachedText& GUI::getCachedText(const std::string& text, TTF_Font* font, SDL_Color color)
{
    for (auto& cached : cached_text) {
        if (cached.text == text && cached.r == color.r && cached.g == color.g &&
            cached.b == color.b)
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

    CachedText cached = {texture, surface->w, surface->h, text, color.r, color.g, color.b};
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

    std::string bg;

    if (system == "") {
        bg = cfg.theme_path + "skin/bg.png";
    } else {
        bg = "/mnt/SDCARD/Backgrounds/" + cfg.backgrounds_theme + "/" + system + ".png";
    }
    render_bg_image(bg);
}
