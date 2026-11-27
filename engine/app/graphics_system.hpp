#ifndef WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP
#define WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP

#include "platform/window.hpp"
#include "render/graphics_api.hpp"
#include "render/renderer.hpp"

#include <memory>
#include <optional>
#include <vector>

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
    openWindow(const platform::Window::Settings& settings) = 0;
    // The graphics APIs this computer supports, among the ones Wolfen names (never
    // GraphicsApi::Other). Needs no window.
    [[nodiscard]] virtual std::vector<render::GraphicsApi> availableGraphicsApis() const = 0;
    // Starts a renderer on the given graphics API and no other one, or on the best one for this
    // computer (falling back to another if it fails) when none is given. Throws StartupError when
    // it cannot start. The renderer must be destroyed before the window it draws into.
    virtual std::unique_ptr<render::Renderer>
    startRenderer(platform::Window& window, std::optional<render::GraphicsApi> graphicsApi) = 0;
    // Starts a renderer on the given graphics API, on a CPU device (a software rasterizer) and
    // nowhere else. Throws StartupError when it cannot, including when it would start on a graphics
    // card instead.
    virtual std::unique_ptr<render::Renderer> startCpuRenderer(platform::Window& window,
                                                               render::GraphicsApi graphicsApi) = 0;
};

} // namespace wolfen::engine::app

#endif // WOLFEN_ENGINE_APP_GRAPHICS_SYSTEM_HPP
