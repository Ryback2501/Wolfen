#include "app/sdl_bgfx_system.hpp"

#include "platform/sdl_window.hpp"
#include "render/bgfx_renderer.hpp"

namespace wolfen::engine::app {

std::unique_ptr<platform::Window>
SdlBgfxSystem::openWindow(const platform::Window::Settings& settings) {
    return std::make_unique<platform::SdlWindow>(settings);
}

std::vector<render::GraphicsApi> SdlBgfxSystem::availableBackends() const {
    return render::BgfxRenderer::availableBackends();
}

std::unique_ptr<render::Renderer>
SdlBgfxSystem::startRenderer(platform::Window& window, std::optional<render::GraphicsApi> backend) {
    return std::make_unique<render::BgfxRenderer>(window, backend);
}

std::unique_ptr<render::Renderer> SdlBgfxSystem::startCpuRenderer(platform::Window& window,
                                                                  render::GraphicsApi backend) {
    return std::make_unique<render::BgfxRenderer>(window, backend,
                                                  render::BgfxRenderer::Device::Cpu);
}

} // namespace wolfen::engine::app
