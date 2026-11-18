#ifndef WOLFEN_ENGINE_CORE_COLOUR_HPP
#define WOLFEN_ENGINE_CORE_COLOUR_HPP

#include <cstdint>

namespace wolfen::engine {

// An RGBA colour with 8 bits per channel.
struct Colour {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{255};

    friend bool operator==(const Colour&, const Colour&) = default;
};

} // namespace wolfen::engine

#endif // WOLFEN_ENGINE_CORE_COLOUR_HPP
