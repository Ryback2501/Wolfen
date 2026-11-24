#include "app/sdl_bgfx_system.hpp"
#include "launcher.hpp"
#include "platform/game_folder.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    wolfen::engine::app::SdlBgfxSystem graphics;
    wolfen::game::Launcher launcher{std::cout, std::cerr, graphics,
                                    wolfen::engine::platform::GameFolder::path()};
    return launcher.run(argc, argv);
}
