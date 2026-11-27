#include "platform/sdl_window.hpp"

#include "core/point.hpp"
#include "core/startup_error.hpp"
#include "platform/screen.hpp"
#include "platform/screen_list.hpp"
#include "platform/window_mode.hpp"

#include <SDL3/SDL.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wolfen::engine::platform {

namespace {

std::string sdlError(std::string_view whatFailed) {
    return std::string{whatFailed} + ": " + SDL_GetError();
}

// Adds the Window::Event an SDL event stands for, if any.
void addWindowEvent(const SDL_Event& event, std::vector<Window::Event>& events) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        events.push_back(Window::Event::CloseRequested);
        break;
    case SDL_EVENT_KEY_DOWN:
        // A key held down repeats; only the first press counts.
        if (event.key.repeat) {
            break;
        }
        if (event.key.key == SDLK_ESCAPE) {
            events.push_back(Window::Event::EscapePressed);
        } else if (event.key.key == SDLK_F11) {
            events.push_back(Window::Event::FullscreenKeyPressed);
        }
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        events.push_back(Window::Event::Resized);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_OCCLUDED:
    case SDL_EVENT_WINDOW_HIDDEN:
        events.push_back(Window::Event::Hidden);
        break;
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_SHOWN:
        events.push_back(Window::Event::Shown);
        break;
    default:
        break;
    }
}

// The screen as the system knows it: its name (empty when it has none) and its top-left corner on
// the desktop (unknown when the system cannot say).
Screen screenOf(SDL_DisplayID display) {
    Screen screen;
    if (const char* name = SDL_GetDisplayName(display); name != nullptr) {
        screen.name = name;
    }
    if (SDL_Rect bounds; SDL_GetDisplayBounds(display, &bounds)) {
        screen.position = Point{.x = bounds.x, .y = bounds.y};
    }
    return screen;
}

// The connected screen that matches the saved one (see ScreenList), or the primary screen.
SDL_DisplayID displayFor(const Screen& savedScreen) {
    int displayCount = 0;
    const std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
        SDL_GetDisplays(&displayCount), &SDL_free};
    if (displays == nullptr) {
        return SDL_GetPrimaryDisplay();
    }
    const std::span<const SDL_DisplayID> connectedDisplays{displays.get(),
                                                           static_cast<std::size_t>(displayCount)};
    std::vector<Screen> screens;
    screens.reserve(connectedDisplays.size());
    for (const SDL_DisplayID display : connectedDisplays) {
        screens.push_back(screenOf(display));
    }
    const std::optional<std::size_t> index = ScreenList{std::move(screens)}.indexOf(savedScreen);
    return index ? connectedDisplays[*index] : SDL_GetPrimaryDisplay();
}

} // namespace

SdlWindow::SdlWindow(const Window::Settings& settings) {
    if (settings.windowMode == WindowMode::Fullscreen) {
        firstWindowedSize_ = settings.windowedSize;
    }
#ifdef __linux__
    // Wayland first, X11 as the fallback. On a Wayland desktop the X11 route (through XWayland)
    // crashes bgfx's OpenGL renderer: EGL has to guess the display type and guesses Wayland.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw StartupError{sdlError("could not start the video system")};
    }

    const SDL_PropertiesID properties = SDL_CreateProperties();
    const auto centredPosition =
        static_cast<std::int64_t>(SDL_WINDOWPOS_CENTERED_DISPLAY(displayFor(settings.screen)));
    SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, settings.title.c_str());
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                          settings.windowedSize.width);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                          settings.windowedSize.height);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, centredPosition);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, centredPosition);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
                           settings.isResizable);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    // Fullscreen covers the screen that holds the window's centre: the chosen one, as the window
    // is centred there. No fullscreen mode is ever set, so it is borderless at the screen's own
    // resolution and the screen's mode never changes.
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN,
                           settings.windowMode == WindowMode::Fullscreen);
    sdlWindow_ = SDL_CreateWindowWithProperties(properties);
    SDL_DestroyProperties(properties);

    if (sdlWindow_ == nullptr) {
        const std::string message = sdlError("could not open the window");
        SDL_Quit();
        throw StartupError{message};
    }
}

SdlWindow::~SdlWindow() {
    SDL_DestroyWindow(sdlWindow_);
    SDL_Quit();
}

std::vector<Window::Event> SdlWindow::pollEvents() {
    std::vector<Window::Event> events;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN) {
            firstWindowedSize_.reset();
            windowedPlacement_.reset();
        }
        addWindowEvent(event, events);
    }
    return events;
}

std::vector<Window::Event> SdlWindow::waitEvents() {
    // Sleeps until there is an event and leaves it in the queue, so pollEvents() takes it with the
    // rest. If waiting fails, the queue is polled as it is.
    SDL_WaitEvent(nullptr);
    return pollEvents();
}

Size SdlWindow::pixelSize() const {
    Size size;
    SDL_GetWindowSizeInPixels(sdlWindow_, &size.width, &size.height);
    return size;
}

void SdlWindow::setTitle(std::string_view title) {
    SDL_SetWindowTitle(sdlWindow_, std::string{title}.c_str());
}

void SdlWindow::switchFullscreen() {
    if ((SDL_GetWindowFlags(sdlWindow_) & SDL_WINDOW_FULLSCREEN) != 0) {
        leaveFullscreen();
    } else {
        enterFullscreen();
    }
}

void SdlWindow::enterFullscreen() {
    WindowedPlacement placement;
    SDL_GetWindowPosition(sdlWindow_, &placement.x, &placement.y);
    SDL_GetWindowSize(sdlWindow_, &placement.size.width, &placement.size.height);
    placement.isMaximized = (SDL_GetWindowFlags(sdlWindow_) & SDL_WINDOW_MAXIMIZED) != 0;
    windowedPlacement_ = placement;

    // Borderless, on the screen that holds the window's centre (see the constructor).
    SDL_SetWindowFullscreen(sdlWindow_, true);
    // Some systems switch a moment later; waiting keeps the window's state up to date for the next
    // switch.
    SDL_SyncWindow(sdlWindow_);
}

void SdlWindow::leaveFullscreen() {
    const SDL_DisplayID display = SDL_GetDisplayForWindow(sdlWindow_);
    SDL_SetWindowFullscreen(sdlWindow_, false);
    // Wait until the window has left fullscreen, so the requests below apply to the window.
    SDL_SyncWindow(sdlWindow_);

    // The system may ignore a position (Wayland always does): nothing is reported or retried.
    if (windowedPlacement_) {
        if (windowedPlacement_->isMaximized) {
            SDL_MaximizeWindow(sdlWindow_);
        } else {
            SDL_SetWindowSize(sdlWindow_, windowedPlacement_->size.width,
                              windowedPlacement_->size.height);
            SDL_SetWindowPosition(sdlWindow_, windowedPlacement_->x, windowedPlacement_->y);
        }
    } else if (firstWindowedSize_) {
        const auto centredPosition = static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(display));
        SDL_SetWindowSize(sdlWindow_, firstWindowedSize_->width, firstWindowedSize_->height);
        SDL_SetWindowPosition(sdlWindow_, centredPosition, centredPosition);
    }
    SDL_SyncWindow(sdlWindow_);
}

Screen SdlWindow::screen() const {
    const SDL_DisplayID display = SDL_GetDisplayForWindow(sdlWindow_);
    return display == 0 ? Screen{} : screenOf(display);
}

Window::NativeHandles SdlWindow::nativeHandles() const {
    const SDL_PropertiesID properties = SDL_GetWindowProperties(sdlWindow_);
#ifdef _WIN32
    return {.kind = Window::NativeHandles::Kind::Windows,
            .windowHandle =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr),
            .displayConnection = nullptr};
#elif defined(__APPLE__)
    return {.kind = Window::NativeHandles::Kind::Cocoa,
            .windowHandle =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr),
            .displayConnection = nullptr};
#else
    const std::string_view videoDriver{SDL_GetCurrentVideoDriver()};
    if (videoDriver == "wayland") {
        return {.kind = Window::NativeHandles::Kind::Wayland,
                .windowHandle = SDL_GetPointerProperty(
                    properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr),
                .displayConnection = SDL_GetPointerProperty(
                    properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr)};
    }
    if (videoDriver != "x11") {
        return {}; // an unexpected video driver: no handle a renderer could use
    }
    // An X11 window is a number, which the renderer expects in a pointer-sized handle.
    const auto x11Window = static_cast<std::uintptr_t>(
        SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    return {.kind = Window::NativeHandles::Kind::X11,
            .windowHandle = std::bit_cast<void*>(x11Window),
            .displayConnection =
                SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr)};
#endif
}

} // namespace wolfen::engine::platform
