#include "app/sdl_bgfx_system.hpp"
#include "launcher.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    wolfen::engine::app::SdlBgfxSystem graphics;
    wolfen::game::Launcher launcher{std::cout, std::cerr, graphics};
    return launcher.run(argc, argv);
}
