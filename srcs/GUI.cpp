#include "GUI.h"
#include "utils.h"

#include <SDL.h>
#include <fcntl.h>
#include <iostream>
#include <linux/fb.h>
#include <map>
#include <sys/ioctl.h>
#include <sys/mman.h>

GUI::GUI()
    : cfg(Config::getInstance())
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

    window = SDL_CreateWindow("Game Timer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320,
        200, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
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
    // Wait the window to be resized to fullscreen
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
    }

    SDL_GetRendererOutputSize(renderer, &Width, &Height);
    std::cout << "Width: " << Width << ", Height: " << Height << std::endl;

    if (TTF_Init()) {
        std::cerr << "Failed to init ttf: " << TTF_GetError() << std::endl;
        return -1;
    }

    return 0;
}

GUI::~GUI()
{
    SDL_JoystickClose(joystick);

    for (auto& texture : image_cache)
        if (texture.second.texture)
            SDL_DestroyTexture(texture.second.texture);
    image_cache.clear();

    for (auto& entry : cached_text)
        if (entry.texture)
            SDL_DestroyTexture(entry.texture);
    cached_text.clear();

    delete_background_texture();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    for (auto& font : fonts)
        TTF_CloseFont(font.second);

    SDL_Quit();
}

void GUI::draw_green_dot(int x, int y, int radius)
{
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255); // vert
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            if (dx * dx + dy * dy <= radius * radius)
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
        }
    }
}

void GUI::draw_circle(int x, int y, int radius, SDL_Color color, bool filled)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (filled) {
        // Filled interior
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx * dx + dy * dy <= radius * radius)
                    SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
        // Draw outline in unselected color (visual contour)
        SDL_Color outline = cfg.unselect_color;
        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
        int  d = 3 - 2 * radius;
        int  cx = 0;
        int  cy = radius;
        auto draw8 = [&](int px, int py) {
            SDL_RenderDrawPoint(renderer, x + px, y + py);
            SDL_RenderDrawPoint(renderer, x - px, y + py);
            SDL_RenderDrawPoint(renderer, x + px, y - py);
            SDL_RenderDrawPoint(renderer, x - px, y - py);
            SDL_RenderDrawPoint(renderer, x + py, y + px);
            SDL_RenderDrawPoint(renderer, x - py, y + px);
            SDL_RenderDrawPoint(renderer, x + py, y - px);
            SDL_RenderDrawPoint(renderer, x - py, y - px);
        };
        while (cx <= cy) {
            draw8(cx, cy);
            if (d < 0) {
                d += 4 * cx + 6;
            } else {
                d += 4 * (cx - cy) + 10;
                cy--;
            }
            cx++;
        }
    } else { // outline only
        int  d = 3 - 2 * radius;
        int  cx = 0;
        int  cy = radius;
        auto draw8 = [&](int px, int py) {
            SDL_RenderDrawPoint(renderer, x + px, y + py);
            SDL_RenderDrawPoint(renderer, x - px, y + py);
            SDL_RenderDrawPoint(renderer, x + px, y - py);
            SDL_RenderDrawPoint(renderer, x - px, y - py);
            SDL_RenderDrawPoint(renderer, x + py, y + px);
            SDL_RenderDrawPoint(renderer, x - py, y + px);
            SDL_RenderDrawPoint(renderer, x + py, y - px);
            SDL_RenderDrawPoint(renderer, x - py, y - px);
        };
        while (cx <= cy) {
            draw8(cx, cy);
            if (d < 0) {
                d = d + 4 * cx + 6;
            } else {
                d = d + 4 * (cx - cy) + 10;
                cy--;
            }
            cx++;
        }
    }
}

void GUI::draw_checkmark(int x, int y, int size, SDL_Color color)
{
    // Simple two-segment checkmark using Bresenham lines
    auto draw_line = [&](int x0, int y0, int x1, int y1) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;
        while (true) {
            SDL_RenderDrawPoint(renderer, x0, y0);
            if (x0 == x1 && y0 == y1)
                break;
            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    };
    int w = size;
    int h = size;
    // Anchor points for a nice checkmark
    int xA = x - w / 3;
    int yA = y;
    int xB = x - w / 10;
    int yB = y + h / 3;
    int xC = x + w / 2;
    int yC = y - h / 2;
    draw_line(xA, yA, xB, yB);
    draw_line(xB, yB, xC, yC);
}

TTF_Font* GUI::get_font(int size)
{
    if (fonts.find(size) == fonts.end()) {
        fonts[size] = TTF_OpenFont((cfg.theme_path + cfg.theme.font).c_str(), size * 4 / 3);
        if (!fonts[size]) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            return nullptr;
        }
    }
    return fonts[size];
}

/**
 * @brief Maps SDL events to our custom InputAction enum.
 *
 * @details This function translates various SDL events (joystick, keyboard, quit)
 * into a more abstract representation of user input actions. It handles
 * joystick axis motion for triggers, hat switches for directional input,
 * button presses for actions, and keyboard presses (if enabled) for
 * similar actions.
 *
 * @param e The SDL_Event to be mapped.
 * @return The corresponding InputAction enum value. Returns InputAction::None
 *         if the event does not map to a recognized action.
 */
InputAction GUI::map_input(const SDL_Event& e)
{
    // Handle triggers as axes for ZL (lefttrigger, a2) and ZR (righttrigger, a5)
    // Typical threshold for trigger activation
    const int16_t TRIGGER_THRESHOLD = 16000;
    if (e.type == SDL_JOYAXISMOTION) {
        if (e.jaxis.axis == 2 && e.jaxis.value > TRIGGER_THRESHOLD) {
            return InputAction::ZL;
        }
        if (e.jaxis.axis == 5 && e.jaxis.value > TRIGGER_THRESHOLD) {
            return InputAction::ZR;
        }
    }
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
        case 9: return InputAction::ZL;
        case 10: return InputAction::ZR;
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

/**
 * @brief Fullfill renderer with black and SDL_RenderClear
 */
void GUI::clear()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

/**
 * @brief Call SDL_RenderPresent
 */
void GUI::render()
{
    SDL_RenderPresent(renderer);
}

/**
 * @brief Renders an image to the screen.
 *
 * @details This function loads an image from the given path, caches it for future use,
 * and then draws it onto the SDL renderer. It supports various scaling and
 * alignment options.
 *
 * @param image_path The path to the image file.
 * @param x The x-coordinate of the image's top-left corner (or center if IMG_CENTER is set).
 * @param y The y-coordinate of the image's top-left corner (or center if IMG_CENTER is set).
 * @param w The desired width of the image. If 0, the aspect ratio is maintained based on height.
 * @param h The desired height of the image. If 0, the aspect ratio is maintained based on width.
 * @param flags Optional flags to control scaling (IMG_FIT) and alignment (IMG_CENTER).
 * @return A Vec2 struct containing the actual rendered width and height of the image.
 */
Vec2 GUI::render_image(const std::string& image_path, int x, int y, int w, int h, int flags)
{
    if (image_path.empty() || !fs::exists(image_path))
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
        if ((flags & IMG_FIT) && height > width) {
            height = width;
            width = height * cached_texture.width / cached_texture.height;
        }
    } else if (w == 0 && h) {
        width = h * cached_texture.width / cached_texture.height;
        if ((flags & IMG_FIT) && width > height) {
            width = height;
            height = width * cached_texture.height / cached_texture.width;
        }
    } else if (w == 0 && h == 0) {
        width = cached_texture.width;
        height = cached_texture.height;
    }

    int      x0 = (flags & IMG_CENTER) ? x - width / 2 : x;
    int      y0 = (flags & IMG_CENTER) ? y - height / 2 : y;
    SDL_Rect dest_rect = {x0, y0, width, height};
    SDL_RenderCopy(renderer, cached_texture.texture, nullptr, &dest_rect);
    return {width, height};
}

/**
 * @brief Renders a cached text element.
 *
 * @details This function retrieves a pre-rendered text texture from the cache or creates
 * it if it doesn't exist. It then draws the texture onto the SDL renderer at the
 * specified position. It handles text clipping if the rendered text exceeds a
 * given width.
 *
 * @param text The Text object containing the string, size, and color.
 * @return A reference to the CachedText object (either existing or newly created).
 */
CachedText& GUI::getCachedText(const Text& text)
{
    for (auto& cached : cached_text) {
        if (cached.text == text)
            return cached;
    }

    TTF_Font* font = get_font(text.size);

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.str.c_str(), text.color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }

    CachedText cached = {texture, surface->w, surface->h, text};
    cached_text.push_back(cached);

    SDL_FreeSurface(surface);
    return cached_text.back();
}

/**
 * @brief Renders a text string with multiple colors.
 *
 * @details This function takes a vector of text segments, each with its own string and color,
 * and renders them sequentially. It's useful for creating titles or labels where
 * different parts need distinct colors. The function uses `getCachedText` to efficiently
 * handle text rendering and caching.
 *
 * @param colored_texts A vector of pairs, where each pair contains a text string and its SDL_Color.
 * @param x The x-coordinate for the start of the text.
 * @param y The y-coordinate for the text.
 * @param font_size The size of the font to use for rendering the text.
 */
void GUI::render_multicolor_text(
    const std::vector<std::pair<std::string, SDL_Color>>& colored_texts, int x, int y,
    int font_size)
{
    int current_x = x;
    for (const auto& text_pair : colored_texts) {
        const std::string& text = text_pair.first;
        SDL_Color          color = text_pair.second;

        CachedText& cached = getCachedText({text, font_size, color});

        if (!cached.texture) {
            std::cerr << "Failed to retrieve or cache text texture." << std::endl;
            return;
        }

        SDL_Rect dest_rect = {current_x, y, cached.width, cached.height};

        SDL_RenderCopy(renderer, cached.texture, nullptr, &dest_rect);

        current_x += cached.width;
    }
}

/**
 * @brief Renders text on the GUI, with optional centering and width clipping.
 *
 * @details This function takes a string and various rendering parameters to display text
 * on the screen. It utilizes a caching mechanism for text textures to improve
 * performance. If the text width exceeds the specified `width`, it will be clipped.
 *
 * @param text The string of text to be rendered.
 * @param x The x-coordinate (horizontal position) where the text will be rendered.
 * @param y The y-coordinate (vertical position) where the text will be rendered.
 * @param font_size The size of the font to use for rendering the text.
 * @param color The SDL_Color struct specifying the color of the text.
 * @param width An optional maximum width for the text. If non-zero and the
 *              cached text's width exceeds this value, the text will be clipped.
 * @param center A boolean flag. If true, the text will be horizontally centered
 *               around the given `x` coordinate.
 * @return A Vec2 object containing the actual width and height of the rendered
 *         text. Returns an empty Vec2 ({0,0}) if there's an error retrieving
 *         or caching the text texture.
 */
Vec2 GUI::render_text(
    const std::string& text, int x, int y, int font_size, SDL_Color color, int width, bool center)
{
    CachedText& cached = getCachedText({text, font_size, color});

    if (!cached.texture) {
        std::cerr << "Failed to retrieve or cache text texture." << std::endl;
        return {};
    }

    if (center) {
        x -= cached.width / 2;
    }
    if (width && cached.width > width) {
        SDL_Rect clip_rect = {0, 0, width, cached.height};
        SDL_Rect render_rect = {x, y, width, cached.height};
        SDL_RenderCopy(renderer, cached.texture, &clip_rect, &render_rect);
    } else {
        SDL_Rect dest_rect = {x, y, cached.width, cached.height};
        SDL_RenderCopy(renderer, cached.texture, nullptr, &dest_rect);
    }
    return {cached.width, cached.height};
}

/**
 * @brief Renders a scrollable text string within a specified width.
 *
 * @details This function displays a given text string. If the text's width exceeds the
 * provided `width`, it will automatically scroll horizontally. The scrolling
 * includes an initial delay, a continuous scroll, and a pause at the end
 * before resetting. Text rendering is cached for performance.
 *
 * @param text The string to be rendered.
 * @param x The x-coordinate for rendering the text.
 * @param y The y-coordinate for rendering the text.
 * @param width The maximum width available for rendering the text. If the text
 *              exceeds this width, it will scroll.
 * @param font_size The size of the font to use for the text.
 * @param color The color of the text.
 */
void GUI::render_scrollable_text(
    const std::string& text, int x, int y, int width, int font_size, SDL_Color color)
{
    static int    offset = 0;
    static Uint32 last_update = 0;
    static bool   end_pause = false;
    static Uint32 end_pause_start = 0;
    CachedText&   cached = getCachedText({text, font_size, color});

    if (scroll_reset) {
        last_update = SDL_GetTicks();
        scroll_reset = false;
        offset = 0;
        end_pause = false;
    }

    if (cached.width <= width) {
        render_text(text, x, y, font_size, color, width);
        return;
    }

    Uint32 current_time = SDL_GetTicks();

    if (end_pause) {
        if (current_time - end_pause_start > 800) {
            scroll_reset = true;
        }
    } else if (current_time - last_update > (offset > 0 ? 16 : 1500)) { // Adjust delays
        last_update = current_time;
        offset += 3; // Scrolling speed
        if (offset > cached.width - width) {
            end_pause = true;
            end_pause_start = current_time;
        }
    }

    SDL_Rect clip_rect = {offset, 0, width, cached.height};
    SDL_Rect render_rect = {x, y, width, cached.height};

    SDL_RenderCopy(renderer, cached.texture, &clip_rect, &render_rect);
}

void GUI::reset_scroll()
{
    scroll_reset = true;
}

/**
 * @brief Renders the background image for the GUI.
 *
 * @details This function is responsible for displaying the correct background image
 * based on the current system and user configuration. It first attempts to render a
 * pre-loaded background texture if one exists. If not, it checks for a system-specific
 * background image. If that doesn't exist, it falls back to the default theme background.
 * The function clears the previous frame before rendering the new background.
 *
 * @param system A constant reference to a string containing the name of the
 * system for which the background is to be rendered.
 */
void GUI::render_background(const std::string& system)
{
    clear();
    if (background_texture) {
        SDL_RenderCopy(renderer, background_texture, nullptr, nullptr);
        return;
    }

    std::string bg = "";
    if (system != "All") {
        bg = "/mnt/SDCARD/Backgrounds/" + cfg.backgrounds_theme + "/" + system + ".png";
        if (!fs::exists(bg))
            bg = "";
    }
    if (bg == "") {
        bg = cfg.theme_path + "skin/bg.png";
        if (!fs::exists(bg)) {
            return;
        }
    }

    render_image(bg, Width / 2, Height / 2, Width, Height);
}

/**
 * @brief Displays a keybind icon and its associated text on the GUI.
 *
 * @details This function renders a specific button icon and its corresponding text
 * label at a designated horizontal position on the screen. It is used to show
 * keybinds or control hints at the bottom of the interface.
 * The button icon is rendered first, and its position is used to correctly align
 * the subsequent text.
 *
 * @param btn A constant reference to a string containing the key for the button's
 * icon in the `buttons_icons` map.
 * @param text A constant reference to a string containing the text to be displayed
 * next to the icon.
 * @param x An integer representing the horizontal (x) coordinate for the keybind's
 * position on the screen.
 */
void GUI::display_keybind(const std::string& btn, const std::string& text, int x)
{
    int prevX =
        render_image(cfg.theme_path + "skin/" + buttons_icons[btn], x, Height - 20, 30, 30).x;

    render_text(text, x + prevX / 2 + 4, Height - 32, FONT_MINI_SIZE, cfg.info_color);
};

/**
 * @brief Displays two keybind icons and their associated text on the GUI.
 *
 * @details This function renders a sequence of two button icons followed by a text label
 * at a specified horizontal position on the screen. It's useful for showing key combinations
 * or a pair of controls with a single descriptive label. The icons are rendered sequentially,
 * and their positions are used to correctly align the subsequent text.
 *
 * @param btn1 A constant reference to a string containing the key for the first
 * button's icon in the `buttons_icons` map.
 * @param btn2 A constant reference to a string containing the key for the second
 * button's icon in the `buttons_icons` map.
 * @param text A constant reference to a string containing the text to be displayed
 * next to the icons.
 * @param x An integer representing the horizontal (x) coordinate for the keybind's
 * position on the screen.
 */
void GUI::display_keybind(const std::string& btn1, const std::string& btn2, const std::string& text,

    int x)
{
    int prevX =
        render_image(cfg.theme_path + "skin/" + buttons_icons[btn1], x, Height - 20, 30, 30).x;

    prevX = render_image(
        cfg.theme_path + "skin/" + buttons_icons[btn2], x + prevX + 6, Height - 20, 30, 30)
                .x;
    render_text(text, x + 4 * prevX / 2, Height - 32, FONT_MINI_SIZE, cfg.info_color);
};

/**
 * @brief Captures a screenshot of the current framebuffer and returns it as an SDL_Texture.
 *
 * @details This function captures the current display by directly reading from the
 * Linux framebuffer device (`/dev/fb0`). It first opens the device, gets its screen
 * information, and maps it into memory. It then copies the raw pixel data into a new
 * SDL_Surface, converts the surface to an SDL_Texture, and cleans up the allocated resources.
 * This method is a low-level way to capture the entire screen content, including UI
 * elements not managed by SDL itself.
 *
 * @return A pointer to the newly created SDL_Texture containing the screenshot. Returns
 * `nullptr` if any step of the process (opening the device, mapping memory, or
 * creating the surface/texture) fails.
 */
SDL_Texture* GUI::take_screenshot()
{

    int                      fb_fd = -1;
    struct fb_fix_screeninfo finfo;
    void*                    fb_ptr = NULL;
    SDL_Surface*             surface = NULL;
    SDL_Texture*             texture = NULL;

    fb_fd = open("/dev/fb0", O_RDONLY);
    if (fb_fd == -1)
        std::cerr << "Error: Could not open framebuffer device." << std::endl;

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        std::cerr << "Error: Could not get fixed screen info from framebuffer." << std::endl;
        close(fb_fd);
        return nullptr;
    }
    fb_ptr = mmap(0, finfo.smem_len, PROT_READ, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED) {
        std::cerr << "Error: Could not mmap framebuffer device." << std::endl;
        close(fb_fd);
        return nullptr;
    }

    close(fb_fd);

    Uint32 sdl_pixel_format = SDL_PIXELFORMAT_ARGB8888;
    surface = SDL_CreateRGBSurfaceWithFormat(0, 1280, 720, 32, sdl_pixel_format);
    if (!surface) {
        std::cerr << "Error: Could not create SDL_Surface." << std::endl;
        munmap(fb_ptr, finfo.smem_len);
        return nullptr;
    }
    Uint8* src_row = (Uint8*) fb_ptr;
    Uint8* dst_row = (Uint8*) surface->pixels;

    for (int y = 0; y < 720; ++y) {
        memcpy(dst_row, src_row, finfo.line_length);
        src_row += finfo.line_length;
        dst_row += surface->pitch;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Error: Could not create SDL_Texture from surface." << std::endl;
        return nullptr;
    }

    munmap(fb_ptr, finfo.smem_len);
    SDL_FreeSurface(surface);

    return texture;
}

/**
 * @brief Saves a background texture for future use.
 *
 * @details This function manages the GUI's background texture. It first releases any existing
 * background texture to prevent memory leaks. If a `required_texture` is provided, it is
 * directly assigned as the new background. If `required_texture` is `nullptr`, the function
 * captures the current contents of the renderer's backbuffer, creates a new texture from it,
 * and saves it for subsequent rendering cycles. This is useful for caching a static background
 * to avoid re-rendering it every frame.
 *
 * @param required_texture A pointer to an `SDL_Texture` to be saved as the background.
 * If `nullptr`, the function will capture the current screen contents instead.
 */
void GUI::save_background_texture(SDL_Texture* required_texture)
{
    delete_background_texture();
    if (required_texture) {
        background_texture = required_texture;
        return;
    }

    SDL_Surface* screen_surface =
        SDL_CreateRGBSurfaceWithFormat(0, Width, Height, 32, SDL_PIXELFORMAT_RGBA8888);

    if (!screen_surface) {
        std::cerr << "Failed to create screen surface: " << SDL_GetError() << std::endl;
        return;
    }

    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, screen_surface->pixels,
            screen_surface->pitch) != 0) {
        std::cerr << "Failed to read pixels: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(screen_surface);
        return;
    }

    background_texture = SDL_CreateTextureFromSurface(renderer, screen_surface);
    SDL_FreeSurface(screen_surface); // Free the surface since it's no longer needed
}

void GUI::delete_background_texture()
{

    if (background_texture) {
        SDL_DestroyTexture(background_texture);
        background_texture = nullptr;
    }
}

/**
 * @brief Displays a modal confirmation popup and handles user input.
 *
 * @details This function creates a blocking popup window with a message and "Yes/No" options.
 * It enters a loop that continuously renders the popup and waits for user input. The user
 * can navigate between "Yes" and "No" using left/right inputs and confirm their choice
 * with the A or B button. The function returns a boolean value indicating the user's
 * final choice. The popup includes a background mask and styled buttons to fit the GUI's theme.
 *
 * @param message A constant reference to a string containing the message to be
 * displayed in the popup.
 * @param font_size An integer specifying the font size for the message and button text.
 * @return `true` if the user selects "Yes" (confirms), `false` if the user selects "No"
 * or quits the popup.
 */
bool GUI::confirmation_popup(const std::string& message, int font_size)
{
    bool confirmed = false;
    bool running = true;
    Vec2 prevSize;
    while (running) {
        render_background();
        render_image(
            cfg.theme_path + "skin/float-win-mask.png", Width / 2, Height / 2, Width, Height);
        render_image(cfg.theme_path + "skin/pop-bg.png", Width / 2, Height / 2, 0, 0);
        render_text(message, Width / 2, Height / 3, font_size, cfg.title_color, 0, true);

        prevSize = render_image(cfg.theme_path + "skin/btn-bg-" + (confirmed ? "f" : "n") + ".png",
            Width / 3, 2 * Height / 3);
        render_text("Yes", Width / 3, 2 * Height / 3 - prevSize.y / 2, font_size,
            confirmed ? cfg.selected_color : cfg.unselect_color, 0, true);
        prevSize = render_image(cfg.theme_path + "skin/btn-bg-" + (confirmed ? "n" : "f") + ".png",
            2 * Width / 3, 2 * Height / 3);
        render_text("No", 2 * Width / 3, 2 * Height / 3 - prevSize.y / 2, font_size,
            confirmed ? cfg.unselect_color : cfg.selected_color, 0, true);
        render();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (map_input(e)) {
            case InputAction::Left: confirmed = true; break;
            case InputAction::Right: confirmed = false; break;
            case InputAction::B:
            case InputAction::A:
            case InputAction::Quit: running = false; break;
            default: break;
            }
        }
    }
    return confirmed;
}

/**
 * @brief Displays a non-blocking message popup for a specified duration.
 *
 * @details This function creates and displays a popup window with one or more lines of text.
 * The popup's size is dynamically calculated based on the content to ensure it fits the text
 * properly. It renders a themed background, the text lines, and then enters a timed loop.
 * During this loop, the popup remains visible, but the user can dismiss it early by pressing
 * a specific action button (A, B, or Quit). The function is non-blocking in the sense that
 * it doesn't require a specific user action to proceed after the duration expires.
 *
 * @param duration An integer representing the minimum number of milliseconds the popup
 * should be displayed.
 * @param lines A constant reference to a vector of `Text` objects, where each object
 * contains a string, font size, and color for a line of text in the popup.
 */
void GUI::message_popup(int duration, const std::vector<Text>& lines)
{

    if (lines.empty()) {
        SDL_Delay(duration);
        return;
    }
    render_background();
    int  margin = 20;
    Vec2 win = {0, 2 * margin};

    Vec2 prevSize;
    for (const Text& line : lines) {

        prevSize = render_text(line.str, 0, -2 * line.size, line.size, line.color, 0, false);
        win.x = std::max(win.x, prevSize.x);
        win.y += prevSize.y * 1.2;
    }
    win.x += margin;
    render_image(cfg.theme_path + "skin/bg-menu-05.png", Width / 2, Height / 2, win.x, win.y);

    int current_y = (Height / 2) - win.y / 2 + margin;

    for (const Text& line : lines)
        current_y +=
            1.2 * render_text(line.str, Width * 0.5, current_y, line.size, line.color, 0, true).y;

    // TODO: test this or move start of loop back to start.
    //  goal: avoid recreate identical rendering.
    while (duration > 0) {
        render();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (map_input(e)) {
            case InputAction::B:
            case InputAction::A:
            case InputAction::Quit: duration = 0; break;
            default: break;
            }
        }
        SDL_Delay(15);
        duration -= 15;
    }
}

/**
 * @brief Renders a customizable information window with a title and content.
 *
 * @details This function draws a pop-up style information window at a specified position
 * and size. It begins by rendering a background image for the window. It then displays a
 * title at the top, centered horizontally. Below the title, it iterates through a vector of
 * key-value pairs (the content) and renders each pair as two-colored text. This allows for
 * clear distinction between a label (e.g., "Version:") and its corresponding value
 * (e.g., "1.2.3"). The spacing and positioning of the content are dynamically calculated
 * to fit within the specified dimensions of the window.
 *
 * @param title A string containing the window's title.
 * @param title_size An integer representing the font size for the title.
 * @param content A vector of `std::pair<std::string, std::string>` where each pair
 * represents a line of content with a key and a value.
 * @param content_size An integer representing the font size for the content text.
 * @param x An integer for the horizontal (x) coordinate of the window's center.
 * @param y An integer for the vertical (y) coordinate of the window's center.
 * @param width An integer specifying the width of the window.
 * @param height An integer specifying the height of the window.
 */
void GUI::infos_window(std::string title, int title_size,
    std::vector<std::pair<std::string, std::string>> content, int content_size, int x, int y,
    int width, int height)
{
    render_image(cfg.theme_path + "skin/pop-bg.png", x, y, width, height);
    int y0 = y - height / 2;

    render_text(title, x, y0, title_size, cfg.title_color, 0, true);
    int content_height = height - 3 * title_size / 2;
    y0 = y0 + height - content_height;
    int dy = content_height / content.size();

    for (size_t i = 0; i < content.size(); i++) {
        render_multicolor_text(
            {{content[i].first, cfg.selected_color}, {content[i].second, cfg.info_color}},
            x - width / 2 + 20, y0 + dy * i, content_size);
    }
}

/**
 * @brief Core function for rendering and managing a scrollable selector menu.
 *
 * @details This is a comprehensive function that handles the full lifecycle of a
 * selector-style menu, from rendering to user input management. It dynamically
 * calculates the window's size based on the content and provides features such as a
 * title, scrollable list of labels, a visual scrollbar, and dynamic highlighting of
 * the currently selected item. The function supports both centered and left-aligned
 * layouts. It continuously polls for user input (Up, Down, Left, Right, A, B, Quit)
 * to navigate the list. Pressing 'A' can trigger a corresponding action from a
 * provided map of functions, while 'B' or 'Quit' will exit the selector.
 *
 * @param title A constant reference to a string for the menu's title. An empty string
 * means no title is displayed.
 * @param labels A constant reference to a vector of strings, where each string is
 * a selectable item in the menu.
 * @param max_width The maximum allowed width for the menu in pixels. A value of 0
 * will automatically adjust the width based on content.
 * @param center A boolean flag. If `true`, the menu's content is centered; otherwise,
 * it is left-aligned.
 * @param initial_selected_index The initial index of the item that should be selected.
 * @param actions A constant reference to a map of strings to functions. The string
 * key must match a label in the `labels` vector. The corresponding function is
 * executed when the 'A' button is pressed on that label. The function should return
 * `true` to exit the menu or `false` to stay in the menu.
 * @return A `std::pair<bool, size_t>`. The boolean is `true` if the user wants to
 * exit the menu (by pressing B or Quit) or if an action returns `true`. The `size_t`
 * is the index of the last selected item.
 */
std::pair<MenuResult, size_t> GUI::_selector_core(const std::string& title,
    const std::vector<std::string>& labels, size_t max_width, bool center,
    size_t                                                    initial_selected_index,
    const std::map<std::string, std::function<MenuResult()>>& actions)
{
    bool   running = true;
    size_t selected_index = initial_selected_index;
    size_t list_size = labels.size();
    size_t lines = std::min(static_cast<int>(list_size), title.empty() ? 11 : 10);

    size_t dy = 60; // button : 52 +  8 padding

    // Calculate width according to inputs length and max_width
    int maxStrLen = 0;
    if (!title.empty())
        maxStrLen =
            render_text(title, 0, -2 * FONT_BIG_SIZE, FONT_BIG_SIZE, cfg.title_color, 0, false).x;
    for (const auto& label : labels)
        maxStrLen = std::max(maxStrLen, render_text(label, 0, -2 * FONT_MIDDLE_SIZE,
                                            FONT_MIDDLE_SIZE, cfg.selected_color, 0, false)
                                            .x);
    if (max_width == 0)
        max_width = Width - 50;
    int width = std::min(static_cast<int>(maxStrLen), static_cast<int>(max_width));
    int height = dy * lines + 8;

    int x0 = static_cast<int>(Width / 2) - (center ? 0 : width / 2);
    int y0 = static_cast<int>(Height / 2) - height / 2 + 8;

    size_t scrollbar_size =
        list_size > lines ? (Height - y0) / std::min(static_cast<int>(list_size), 50) : 0;

    while (running) {
        int  y = y0;
        Vec2 prevSize;
        render_background();
        render_image(
            cfg.theme_path + "skin/background-mask.png", Width / 2, Height / 2, Width, Height);
        if (!title.empty()) {
            render_image(cfg.theme_path + "skin/bg-menu-05.png", Width / 2, Height / 2 - 30,
                width + 50, height + 60);
            render_text(title, Width / 2, y0 - 60, FONT_BIG_SIZE, cfg.title_color, 0, true);
        } else {
            render_image(
                cfg.theme_path + "skin/bg-menu-05.png", Width / 2, Height / 2, width + 50, height);
        }

        size_t first =
            (list_size <= lines)
                ? 0
                : std::max(0, static_cast<int>(selected_index) - static_cast<int>(lines) / 2);
        size_t last = std::min(first + lines, list_size);

        if (scrollbar_size)
            render_image(std::string(APP_DIR) + "/.assets/scroll-v.svg", (Width + width) / 2 - 12,
                y + (height - scrollbar_size / 2) * selected_index / list_size, 50, scrollbar_size,
                IMG_NONE);

        auto line = labels.begin() + first;
        for (size_t j = first; j < last; ++j) {
            SDL_Color color = (j == selected_index) ? cfg.selected_color : cfg.unselect_color;

            prevSize = render_image(cfg.theme_path + "skin/list-item-1line-sort-bg-" +
                                        (j == selected_index ? "f" : "n") + ".png",
                x0, y + (center ? (dy - 8) / 2 : 0), width + 4, dy - 8,
                center ? IMG_CENTER : IMG_NONE);

            if (j == selected_index && !center)
                render_scrollable_text(*line, x0, y + 2, prevSize.x - 5, FONT_MIDDLE_SIZE, color);
            else
                render_text(*line, x0, y + 2, FONT_MIDDLE_SIZE, color, prevSize.x - 5,
                    center ? true : false);

            line++;
            y += dy;
        }
        render();

        size_t    prev_selected_index = selected_index;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {

            switch (map_input(e)) {
            case InputAction::Up: {
                if (selected_index > 0)
                    selected_index--;
                break;
            }
            case InputAction::Down: {
                if (selected_index < list_size - 1)
                    selected_index++;
                break;
            }
            case InputAction::Left:
                selected_index = selected_index > lines ? selected_index - lines : 0;
                break;
            case InputAction::Right:
                selected_index = list_size > lines && selected_index < list_size - lines
                                     ? selected_index + lines
                                     : static_cast<int>(list_size) - 1;
                break;

            case InputAction::B: return {MenuResult::ExitCurrent, selected_index};
            case InputAction::Quit: return {MenuResult::ExitAll, selected_index};
            case InputAction::A: {
                if (actions.empty())
                    return {MenuResult::Continue, selected_index};
                const std::string& chosen_label = labels[selected_index];
                auto               it = actions.find(chosen_label);
                if (it != actions.end()) {
                    MenuResult result = it->second();
                    if (result != MenuResult::Continue)
                        return {result, selected_index};
                }
            } break;
            default: break;
            }
        }

        if (prev_selected_index != selected_index)
            reset_scroll();
    }
    return {MenuResult::Continue, selected_index};
}

/**
 * @brief Displays an interactive file selector menu for a given directory.
 *
 * @details This function provides a recursive file and directory selection interface.
 * It populates a list with the contents of the specified `location` and uses the
 * `_selector_core` function to display a selectable menu to the user. If `hide_empties` is
 * true, it filters out directories that are empty or contain only a parent directory entry.
 * When the user selects a directory, the function recursively calls itself to navigate
 * into the new directory. When a file is selected, it returns the shortened path to that file.
 * The function returns an empty string if the user exits the selector without making a
 * file selection.
 * TODO: Only accept a specific type of file (add vector<string> as argument with filetype accepted)
 *
 * @param location The `fs::path` representing the directory to display.
 * @param hide_empties A boolean flag indicating whether to hide empty directories from the
 * list.
 * @return A `std::string` containing the shortened path of the selected file, or an
 * empty string if no file was selected.
 */
const std::string GUI::file_selector(fs::path location, bool hide_empties)
{
    std::vector<std::string> content = utils::get_directory_content(location, true);
    if (hide_empties) {
        content.erase(std::remove_if(content.begin(), content.end(),
                          [location](const std::string& sub) {
                              if (sub == "..")
                                  return false;
                              std::string next = location.string() + "/" + sub;
                              if (fs::status(next).type() == fs::file_type::regular)
                                  return false;
                              if (fs::is_empty(fs::path(next)))
                                  return true;
                              std::vector<std::string> sub_content =
                                  utils::get_directory_content(next, true);
                              return sub_content.size() < 2;
                          }),
            content.end());
    }

    std::pair<MenuResult, size_t> result =
        _selector_core("File explorer", content, 0, false, 0, {});

    if (result.first != MenuResult::Continue)
        return "";
    std::string next = location.string() + "/" + content[result.second];
    if (fs::status(next).type() == fs::file_type::directory)
        return file_selector(next, true);
    return utils::shorten_file_path(next);
}

/**
 * @brief Displays a selectable menu to choose a string from a list.
 *
 * @details This function presents a user with a menu to select one string from a
 * provided list. It is a wrapper around the `_selector_core` function, simplifying
 * its use for string selection. It displays the menu with a given title, a list of
 * labels, and options for maximum width and centering. The function returns the
 * selected string upon confirmation or an empty string if the user exits the menu
 * without making a selection.
 *
 * @param title A constant reference to a string for the menu's title.
 * @param labels A constant reference to a vector of strings representing the selectable
 * options.
 * @param max_width The maximum allowed width for the menu in pixels. A value of 0
 * will automatically adjust the width based on content.
 * @param center A boolean flag. If `true`, the menu's content is centered; otherwise,
 * it is left-aligned.
 * @return A `const std::string&` of the selected label, or an empty string if the
 * selection was canceled.
 */
const std::string GUI::string_selector(
    const std::string& title, const std::vector<std::string>& labels, size_t max_width, bool center)
{
    std::pair<MenuResult, size_t> result = _selector_core(title, labels, max_width, center, 0, {});
    if (result.first != MenuResult::Continue)
        return "";
    return labels[result.second];
}

/**
 * @brief Displays and manages a standard menu with selectable items.
 *
 * @details This function creates a full-featured, interactive menu using a vector of
 * string-function pairs. It maps these pairs into separate data structures (`labels` and
 * `action_map`) to be used by the underlying `_selector_core` function. The function
 * enters a loop that continuously calls `_selector_core` to render the menu and handle
 * user input. When the user selects an item (by pressing 'A'), the corresponding function
 * is executed. The menu will persist until an action function returns `true`, or the user
 * presses 'B' or a quit button, signaling the menu to close and the function to return.
 * The `current_selected_index` is preserved across redraws to maintain the user's position.
 *
 * @param menu_items A `std::vector` of `std::pair<std::string, std::function<bool()>>`.
 * Each pair consists of a string to be used as a menu item label and a function to be
 * executed when that item is selected. The function should return `true` to exit the menu
 * or `false` to remain in it.
 */
MenuResult GUI::menu(const std::string&                              title,
    std::vector<std::pair<std::string, std::function<MenuResult()>>> menu_items)
{
    size_t current_selected_index = 0;

    std::vector<std::string>                           labels;
    std::map<std::string, std::function<MenuResult()>> action_map;
    for (const auto& item : menu_items) {
        labels.push_back(item.first);
        action_map[item.first] = item.second;
    }

    while (true) {
        std::pair<MenuResult, size_t> result =
            _selector_core(title, labels, Width / 3, true, current_selected_index, action_map);

        current_selected_index = result.second;

        if (result.first == MenuResult::ExitCurrent)
            return MenuResult::Continue;
        if (result.first == MenuResult::ExitAll)
            return MenuResult::ExitAll;
    }
}
