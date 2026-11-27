#ifndef WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
#define WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP

#include "render/graphics_api.hpp"

#include <string_view>

namespace wolfen::engine::render {

// The name of a graphics API as shown to people, for example in the window title. It needs no
// bgfx header, so the game and the tests can use it. The bgfx details of each graphics API are in
// RendererTable, which only engine/render/ may use.
class RendererName {
  public:
    // engineName is the rendering library's own name for the graphics API, used for Other.
    static std::string_view display(GraphicsApi graphicsApi, std::string_view engineName);
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
