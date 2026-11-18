#include "app/sdl_bgfx_system.hpp"
#include "game.hpp"

#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    // The start options, without the program's own name.
    const std::span<char*> words{argv, static_cast<std::size_t>(argc)};
    const std::vector<std::string_view> args(words.empty() ? words.end() : words.begin() + 1,
                                             words.end());

    wolfen::engine::app::SdlBgfxSystem graphics;
    wolfen::game::Game game{std::cout, std::cerr, graphics};
    return game.run(args);
}
