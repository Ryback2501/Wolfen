#include "app/sdl_bgfx_system.hpp"

#include "platform/sdl_window.hpp"
#include "render/bgfx_renderer.hpp"

namespace wolfen::engine::app {

std::unique_ptr<platform::Window>
SdlBgfxSystem::openWindow(const platform::WindowSettings& settings) {
    return std::make_unique<platform::SdlWindow>(settings);
}

std::vector<render::Backend> SdlBgfxSystem::availableRenderers() const {
    return {};
}

std::unique_ptr<render::Renderer>
SdlBgfxSystem::startRenderer(platform::Window& window, std::optional<render::Backend> /*only*/) {
    return std::make_unique<render::BgfxRenderer>(window);
}

} // namespace wolfen::engine::app
