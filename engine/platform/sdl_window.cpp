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

// Adds the WindowEvent an SDL event stands for, if any.
void addWindowEvent(const SDL_Event& event, std::vector<WindowEvent>& events) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        events.push_back(WindowEvent::CloseRequested);
        break;
    case SDL_EVENT_KEY_DOWN:
        // A key held down repeats; only the first press counts.
        if (event.key.repeat) {
            break;
        }
        if (event.key.key == SDLK_ESCAPE) {
            events.push_back(WindowEvent::EscapePressed);
        } else if (event.key.key == SDLK_F11) {
            events.push_back(WindowEvent::FullscreenKeyPressed);
        }
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        events.push_back(WindowEvent::Resized);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_OCCLUDED:
    case SDL_EVENT_WINDOW_HIDDEN:
        events.push_back(WindowEvent::Hidden);
        break;
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_SHOWN:
        events.push_back(WindowEvent::Shown);
        break;
    default:
        break;
    }
}

} // namespace

SdlWindow::SdlWindow(const WindowSettings& settings) : firstWindowedSize_{settings.size} {
#ifdef __linux__
    // Wayland first, X11 as the fallback. On a Wayland desktop the X11 route (through XWayland)
    // crashes bgfx's OpenGL renderer: EGL has to guess the display type and guesses Wayland.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
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
    // Fullscreen covers the screen that holds the window's centre: the primary one, as the window
    // is centred there. No fullscreen mode is ever set, so it is borderless at the screen's own
    // resolution and the screen's mode never changes.
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN,
                           settings.mode == WindowMode::Fullscreen);
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
        addWindowEvent(event, events);
    }
    return events;
}

std::vector<WindowEvent> SdlWindow::waitEvents() {
    // Sleeps until there is an event and leaves it in the queue, so pollEvents() takes it with the
    // rest. If waiting fails, the queue is polled as it is.
    SDL_WaitEvent(nullptr);
    return pollEvents();
}

Size SdlWindow::pixelSize() const {
    Size size;
    SDL_GetWindowSizeInPixels(window_, &size.width, &size.height);
    return size;
}

void SdlWindow::setTitle(std::string_view title) {
    SDL_SetWindowTitle(window_, std::string{title}.c_str());
}

void SdlWindow::switchFullscreen() {
    if ((SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0) {
        leaveFullscreen();
    } else {
        enterFullscreen();
    }
}

void SdlWindow::enterFullscreen() {
    WindowedPlacement placement;
    SDL_GetWindowPosition(window_, &placement.x, &placement.y);
    SDL_GetWindowSize(window_, &placement.size.width, &placement.size.height);
    placement.isMaximized = (SDL_GetWindowFlags(window_) & SDL_WINDOW_MAXIMIZED) != 0;
    windowedPlacement_ = placement;

    // Borderless, on the screen that holds the window's centre (see the constructor).
    SDL_SetWindowFullscreen(window_, true);
    // Some systems switch a moment later; waiting keeps the window's state up to date for the next
    // switch.
    SDL_SyncWindow(window_);
}

void SdlWindow::leaveFullscreen() {
    const SDL_DisplayID screen = SDL_GetDisplayForWindow(window_);
    SDL_SetWindowFullscreen(window_, false);
    // Wait until the window has left fullscreen, so the requests below apply to the window.
    SDL_SyncWindow(window_);

    // The system may ignore a position (Wayland always does): nothing is reported or retried.
    if (!windowedPlacement_) {
        const auto centred = static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(screen));
        SDL_SetWindowSize(window_, firstWindowedSize_.width, firstWindowedSize_.height);
        SDL_SetWindowPosition(window_, centred, centred);
    } else if (windowedPlacement_->isMaximized) {
        SDL_MaximizeWindow(window_);
    } else {
        SDL_SetWindowSize(window_, windowedPlacement_->size.width, windowedPlacement_->size.height);
        SDL_SetWindowPosition(window_, windowedPlacement_->x, windowedPlacement_->y);
    }
    SDL_SyncWindow(window_);
}

NativeWindow SdlWindow::nativeWindow() const {
    const SDL_PropertiesID properties = SDL_GetWindowProperties(window_);
#ifdef _WIN32
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
    const std::string_view driver{SDL_GetCurrentVideoDriver()};
    if (driver == "wayland") {
        return {.kind = NativeWindow::Kind::Wayland,
                .window = SDL_GetPointerProperty(properties,
                                                 SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr),
                .display = SDL_GetPointerProperty(
                    properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr)};
    }
    if (driver != "x11") {
        return {}; // an unexpected video driver: no handle a renderer could use
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
