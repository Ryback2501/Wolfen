#ifndef WOLFEN_ENGINE_CORE_SIZE_HPP
#define WOLFEN_ENGINE_CORE_SIZE_HPP

namespace wolfen::engine {

// A width and a height, in pixels.
struct Size {
    int width{};
    int height{};

    friend bool operator==(const Size&, const Size&) = default;
};

} // namespace wolfen::engine

#endif // WOLFEN_ENGINE_CORE_SIZE_HPP
