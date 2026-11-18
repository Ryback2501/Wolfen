#include "render/renderer_name.hpp"

namespace wolfen::engine::render {

std::string_view RendererName::display(Backend /*backend*/, std::string_view /*engineName*/) {
    return {};
}

} // namespace wolfen::engine::render
