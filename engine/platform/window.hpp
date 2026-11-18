#ifndef WOLFEN_ENGINE_PLATFORM_WINDOW_HPP
#define WOLFEN_ENGINE_PLATFORM_WINDOW_HPP

#include "core/size.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wolfen::engine::platform {

// How a window is opened.
struct WindowSettings {
    std::string title;
    Size size;
    bool resizable{true};
};

// Something that happened to the window since the last poll.
enum class WindowEvent : std::uint8_t {
    CloseRequested, // the window's close button
    EscapePressed,
    Resized, // the size in pixels changed; read it with Window::pixelSize()
};

// The operating system's handles for a window, which a renderer needs to draw into it.
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
    [[nodiscard]] virtual Size pixelSize() const = 0;
    virtual void setTitle(std::string_view title) = 0;
    [[nodiscard]] virtual NativeWindow nativeWindow() const = 0;
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_WINDOW_HPP
