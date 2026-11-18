#include "render/renderer_name.hpp"

namespace wolfen::engine::render {

std::string_view RendererName::display(Backend backend, std::string_view engineName) {
    switch (backend) {
    case Backend::Vulkan:
        return "Vulkan";
    case Backend::OpenGL:
        return "OpenGL";
    case Backend::Metal:
        return "Metal";
    case Backend::Direct3D11:
        return "Direct3D 11";
    case Backend::Direct3D12:
        return "Direct3D 12";
    case Backend::Other:
        break;
    }
    return engineName;
}

} // namespace wolfen::engine::render
