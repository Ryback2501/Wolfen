#include "app/sdl_bgfx_system.hpp"
#include "game.hpp"

#include <iostream>

int main() {
    wolfen::engine::app::SdlBgfxSystem graphics;
    wolfen::game::Game game{std::cout, std::cerr, graphics};
    return game.run();
}
