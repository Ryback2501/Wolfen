#include "app/sdl_bgfx_system.hpp"

#include "platform/sdl_window.hpp"
#include "render/bgfx_renderer.hpp"

namespace wolfen::engine::app {

std::unique_ptr<platform::Window>
SdlBgfxSystem::openWindow(const platform::Window::Settings& settings) {
    return std::make_unique<platform::SdlWindow>(settings);
}

std::vector<render::Backend> SdlBgfxSystem::availableBackends() const {
    return render::BgfxRenderer::availableBackends();
}

std::unique_ptr<render::Renderer>
SdlBgfxSystem::startRenderer(platform::Window& window, std::optional<render::Backend> backend) {
    return std::make_unique<render::BgfxRenderer>(window, backend);
}

std::unique_ptr<render::Renderer> SdlBgfxSystem::startCpuRenderer(platform::Window& window,
                                                                  render::Backend backend) {
    return std::make_unique<render::BgfxRenderer>(window, backend, render::DeviceChoice::Cpu);
}

} // namespace wolfen::engine::app
