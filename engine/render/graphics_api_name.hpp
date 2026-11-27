#ifndef WOLFEN_ENGINE_RENDER_GRAPHICS_API_NAME_HPP
#define WOLFEN_ENGINE_RENDER_GRAPHICS_API_NAME_HPP

#include "render/graphics_api.hpp"

#include <string_view>

namespace wolfen::engine::render {

// The name of a graphics API as shown to people, for example in the window title. It needs no
// bgfx header, so the game and the tests can use it. The bgfx details of each graphics API are in
// GraphicsApiTable, which only engine/render/ may use.
class GraphicsApiName {
  public:
    // libraryName is the rendering library's own name for the graphics API, used for Other.
    static std::string_view displayName(GraphicsApi graphicsApi, std::string_view libraryName);
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_GRAPHICS_API_NAME_HPP
