#ifndef WOLFEN_ENGINE_PLATFORM_WINDOW_MODE_HPP
#define WOLFEN_ENGINE_PLATFORM_WINDOW_MODE_HPP

#include <cstdint>

namespace wolfen::engine::platform {

// Whether a window is a normal window or covers its whole screen.
enum class WindowMode : std::uint8_t {
    Windowed,
    Fullscreen, // borderless, at the screen's own resolution; the screen's mode never changes
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_WINDOW_MODE_HPP
