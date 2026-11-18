#include "platform/sdl_window.hpp"

#include "core/startup_error.hpp"

#include <SDL3/SDL.h>

#include <bit>
#include <cstdint>
#include <string>

namespace wolfen::engine::platform {

namespace {

std::string sdlError(std::string_view whatFailed) {
    return std::string{whatFailed} + ": " + SDL_GetError();
}

} // namespace

SdlWindow::SdlWindow(const WindowSettings& settings) {
#if defined(__linux__)
    // X11 first (it also runs on Wayland desktops through XWayland), Wayland as the fallback.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11,wayland");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw StartupError{sdlError("could not start the video system")};
    }

    const SDL_PropertiesID properties = SDL_CreateProperties();
    const auto centred =
        static_cast<std::int64_t>(SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()));
    SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, settings.title.c_str());
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, settings.size.width);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, settings.size.height);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, centred);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, centred);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
                           settings.resizable);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    window_ = SDL_CreateWindowWithProperties(properties);
    SDL_DestroyProperties(properties);

    if (window_ == nullptr) {
        const std::string message = sdlError("could not open the window");
        SDL_Quit();
        throw StartupError{message};
    }
}

SdlWindow::~SdlWindow() {
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

std::vector<WindowEvent> SdlWindow::pollEvents() {
    std::vector<WindowEvent> events;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            events.push_back(WindowEvent::CloseRequested);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE && !event.key.repeat) {
                events.push_back(WindowEvent::EscapePressed);
            }
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            events.push_back(WindowEvent::Resized);
            break;
        default:
            break;
        }
    }
    return events;
}

Size SdlWindow::pixelSize() const {
    Size size;
    SDL_GetWindowSizeInPixels(window_, &size.width, &size.height);
    return size;
}

void SdlWindow::setTitle(std::string_view title) {
    SDL_SetWindowTitle(window_, std::string{title}.c_str());
}

NativeWindow SdlWindow::nativeWindow() const {
    const SDL_PropertiesID properties = SDL_GetWindowProperties(window_);
#if defined(_WIN32)
    return {.kind = NativeWindow::Kind::Windows,
            .window =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr),
            .display = nullptr};
#elif defined(__APPLE__)
    return {.kind = NativeWindow::Kind::Cocoa,
            .window =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr),
            .display = nullptr};
#else
    if (std::string_view{SDL_GetCurrentVideoDriver()} == "wayland") {
        return {.kind = NativeWindow::Kind::Wayland,
                .window = SDL_GetPointerProperty(properties,
                                                 SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr),
                .display = SDL_GetPointerProperty(
                    properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr)};
    }
    // An X11 window is a number, which the renderer expects in a pointer-sized handle.
    const auto x11Window = static_cast<std::uintptr_t>(
        SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    return {.kind = NativeWindow::Kind::X11,
            .window = std::bit_cast<void*>(x11Window),
            .display =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr)};
#endif
}

} // namespace wolfen::engine::platform
