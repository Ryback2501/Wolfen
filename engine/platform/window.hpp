#ifndef WOLFEN_ENGINE_PLATFORM_WINDOW_HPP
#define WOLFEN_ENGINE_PLATFORM_WINDOW_HPP

#include "core/size.hpp"
#include "platform/screen.hpp"
#include "platform/window_mode.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wolfen::engine::platform {

// How a window is opened.
struct WindowSettings {
    std::string title;
    Size size; // in a window; for Fullscreen, the size it gets when it first leaves fullscreen
    bool resizable{true};
    WindowMode mode{WindowMode::Windowed};
    // The screen to open on, centred there or covering it: the connected screen with this name and
    // position, or else the first one with this name; an empty name, or one no connected screen
    // has, means the primary screen. Where the system places windows itself (Linux under Wayland),
    // a window may open elsewhere, and nothing is reported.
    Screen screen;
};

// Something that happened to the window since the last poll.
enum class WindowEvent : std::uint8_t {
    CloseRequested, // the window's close button
    EscapePressed,
    Resized,              // the size in pixels changed; read it with Window::pixelSize()
    Hidden,               // minimized or fully covered: nothing drawn would be seen
    Shown,                // visible again after Hidden
    FullscreenKeyPressed, // F11, once per press: holding it down does not repeat it
};

// The operating system's handles for a window, which a renderer needs to draw into it. A null
// window handle means the window has none a renderer could use.
struct NativeWindow {
    enum class Kind : std::uint8_t { Windows, Cocoa, X11, Wayland };

    Kind kind{};
    void* window{};  // HWND, NSWindow*, X11 Window id, or wl_surface*
    void* display{}; // X11 Display* or wl_display*; null elsewhere
};

// A window on the screen. The window is open for as long as the object lives.
class Window {
  public:
    Window() = default;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;
    virtual ~Window() = default;

    // Everything that happened since the last call, oldest first.
    virtual std::vector<WindowEvent> pollEvents() = 0;
    // Like pollEvents(), but first sleeps until something happens (for a hidden window).
    virtual std::vector<WindowEvent> waitEvents() = 0;
    [[nodiscard]] virtual Size pixelSize() const = 0;
    virtual void setTitle(std::string_view title) = 0;
    // Goes from a window to fullscreen on the screen that holds the window's centre, or from
    // fullscreen back to a window with the size, position and maximized state it had before. A
    // window that opened in fullscreen goes back to its settings' size, centred on its screen.
    virtual void switchFullscreen() = 0;
    // The screen that holds the window's centre (in fullscreen, the screen it covers). Its name is
    // empty when the system gives that screen none.
    [[nodiscard]] virtual Screen screen() const = 0;
    [[nodiscard]] virtual NativeWindow nativeWindow() const = 0;
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_WINDOW_HPP
