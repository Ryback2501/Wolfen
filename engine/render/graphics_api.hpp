#ifndef WOLFEN_ENGINE_RENDER_GRAPHICS_API_HPP
#define WOLFEN_ENGINE_RENDER_GRAPHICS_API_HPP

#include <cstdint>

namespace wolfen::engine::render {

// The graphics APIs Wolfen names itself; every other one is Other.
enum class GraphicsApi : std::uint8_t { Vulkan, OpenGL, Metal, Direct3D11, Direct3D12, Other };

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_GRAPHICS_API_HPP
