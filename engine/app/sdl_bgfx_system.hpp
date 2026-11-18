#ifndef WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP
#define WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP

#include "app/graphics_system.hpp"

namespace wolfen::engine::app {

// The real graphics system: windows from SDL3, renderers from bgfx.
class SdlBgfxSystem final : public GraphicsSystem {
  public:
    std::unique_ptr<platform::Window> openWindow(const platform::WindowSettings& settings) override;
    [[nodiscard]] std::vector<render::Backend> availableRenderers() const override;
    std::unique_ptr<render::Renderer> startRenderer(platform::Window& window,
                                                    std::optional<render::Backend> only) override;
};

} // namespace wolfen::engine::app

#endif // WOLFEN_ENGINE_APP_SDL_BGFX_SYSTEM_HPP
