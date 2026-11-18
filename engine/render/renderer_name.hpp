#ifndef WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
#define WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP

#include <cstdint>
#include <string_view>

namespace wolfen::engine::render {

// The renderer backends Wolfen names itself; every other one is Other.
enum class Backend : std::uint8_t { Vulkan, OpenGL, Metal, Direct3D11, Direct3D12, Other };

// The name of a renderer backend as shown to people, for example in the window title.
class RendererName {
  public:
    // engineName is the rendering library's own name for the backend, used for Other.
    static std::string_view display(Backend backend, std::string_view engineName);
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_RENDERER_NAME_HPP
