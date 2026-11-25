#ifndef WOLFEN_ENGINE_PLATFORM_SCREEN_HPP
#define WOLFEN_ENGINE_PLATFORM_SCREEN_HPP

#include "core/point.hpp"

#include <optional>
#include <string>

namespace wolfen::engine::platform {

// A screen as the system knows it: the name it gives the screen, and where the screen's top-left
// corner sits on the desktop (unknown when the system cannot say). Two screens of the same model
// share a name; their positions tell them apart.
struct Screen {
    std::string name;
    std::optional<Point> position;

    friend bool operator==(const Screen&, const Screen&) = default;
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_SCREEN_HPP
