#ifndef WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
#define WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP

#include "render/backend.hpp"

#include <string_view>

namespace wolfen::engine::render {

// The name of a renderer backend as shown to people, for example in the window title. It needs no
// bgfx header, so the game and the tests can use it. The bgfx details of each backend are in
// RendererTable, which only engine/render/ may use.
class RendererName {
  public:
    // engineName is the rendering library's own name for the backend, used for Other.
    static std::string_view display(Backend backend, std::string_view engineName);
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
