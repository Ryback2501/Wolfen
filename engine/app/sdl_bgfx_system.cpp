#include "app/sdl_bgfx_system.hpp"

#include "core/startup_error.hpp"

namespace wolfen::engine::app {

std::unique_ptr<platform::Window>
SdlBgfxSystem::openWindow(const platform::WindowSettings& /*settings*/) {
    throw StartupError{"opening a window is not built yet"};
}

std::unique_ptr<render::Renderer> SdlBgfxSystem::startRenderer(platform::Window& /*window*/) {
    throw StartupError{"starting a renderer is not built yet"};
}

} // namespace wolfen::engine::app
