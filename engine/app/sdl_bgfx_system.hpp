#ifndef WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP
#define WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP

#include "app/graphics_system.hpp"

namespace wolfen::engine::app {

// The real graphics system: windows from SDL3, renderers from bgfx.
class SdlBgfxSystem final : public GraphicsSystem {
  public:
    std::unique_ptr<platform::Window>
    openWindow(const platform::Window::Settings& settings) override;
    [[nodiscard]] std::vector<render::GraphicsApi> availableBackends() const override;
    std::unique_ptr<render::Renderer>
    startRenderer(platform::Window& window, std::optional<render::GraphicsApi> backend) override;
    std::unique_ptr<render::Renderer> startCpuRenderer(platform::Window& window,
                                                       render::GraphicsApi backend) override;
};

} // namespace wolfen::engine::app

#endif // WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP
