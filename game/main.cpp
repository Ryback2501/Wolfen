#include "app/sdl_bgfx_system.hpp"
#include "launcher.hpp"
#include "platform/game_folder.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    wolfen::engine::app::SdlBgfxSystem graphics;
    const wolfen::engine::platform::GameFolder gameFolder;
    wolfen::game::Launcher launcher{std::cout, std::cerr, graphics, gameFolder.path()};
    return launcher.run(argc, argv);
}
