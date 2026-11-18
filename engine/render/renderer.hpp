#ifndef WOLFEN_ENGINE_RENDER_RENDERER_HPP
#define WOLFEN_ENGINE_RENDER_RENDERER_HPP

#include "core/colour.hpp"
#include "core/size.hpp"

#include <string_view>

namespace wolfen::engine::render {

// Draws into one window. Rendering is possible for as long as the object lives.
class Renderer {
  public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    virtual ~Renderer() = default;

    // The name to show people, for example "Vulkan" (see RendererName).
    [[nodiscard]] virtual std::string_view name() const = 0;
    // The window's size in pixels changed.
    virtual void resize(Size pixelSize) = 0;
    // Draws one frame: the whole window in the background colour.
    virtual void drawFrame(Colour background) = 0;
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_RENDERER_HPP
