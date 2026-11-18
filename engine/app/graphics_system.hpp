#ifndef WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP
#define WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP

#include "platform/window.hpp"
#include "render/renderer.hpp"

#include <memory>

namespace wolfen::engine::app {

// Opens windows and starts renderers for them. The game uses it through this interface, so the
// tests can give it a fake one that needs no screen.
class GraphicsSystem {
  public:
    GraphicsSystem() = default;
    GraphicsSystem(const GraphicsSystem&) = delete;
    GraphicsSystem& operator=(const GraphicsSystem&) = delete;
    GraphicsSystem(GraphicsSystem&&) = delete;
    GraphicsSystem& operator=(GraphicsSystem&&) = delete;
    virtual ~GraphicsSystem() = default;

    // Throws StartupError when the window cannot be opened.
    virtual std::unique_ptr<platform::Window>
    openWindow(const platform::WindowSettings& settings) = 0;
    // Throws StartupError when no renderer can start. The renderer must be destroyed before the
    // window it draws into.
    virtual std::unique_ptr<render::Renderer> startRenderer(platform::Window& window) = 0;
};

} // namespace wolfen::engine::app

#endif // WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP
