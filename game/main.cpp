#include "app/sdl_bgfx_system.hpp"
#include "launcher.hpp"
#include "platform/game_folder.hpp"

#include <iostream>

using wolfen::engine::app::SdlBgfxSystem;
using wolfen::engine::platform::GameFolder;
using wolfen::game::Launcher;

int main(int argc, char* argv[]) {
    SdlBgfxSystem graphicsSystem;
    const GameFolder gameFolder;
    Launcher launcher{std::cout, std::cerr, graphicsSystem, gameFolder.path()};
    return launcher.run(argc, argv);
}
