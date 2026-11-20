#ifndef WOLFEN_ENGINE_RENDER_BACKEND_HPP
#define WOLFEN_ENGINE_RENDER_BACKEND_HPP

#include <cstdint>

namespace wolfen::engine::render {

// The renderer backends Wolfen names itself; every other one is Other.
enum class Backend : std::uint8_t { Vulkan, OpenGL, Metal, Direct3D11, Direct3D12, Other };

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_BACKEND_HPP
