#ifndef WOLFEN_ENGINE_CORE_POINT_HPP
#define WOLFEN_ENGINE_CORE_POINT_HPP

namespace wolfen::engine {

// A place on the desktop or in a window, in pixels from the top-left corner.
struct Point {
    int x{};
    int y{};

    friend bool operator==(const Point&, const Point&) = default;
};

} // namespace wolfen::engine

#endif // WOLFEN_ENGINE_CORE_POINT_HPP
