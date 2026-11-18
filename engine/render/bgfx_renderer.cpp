#include "render/bgfx_renderer.hpp"

#include "core/startup_error.hpp"
#include "render/renderer_name.hpp"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace wolfen::engine::render {

namespace {

constexpr bgfx::ViewId mainView = 0;
constexpr std::uint32_t resetFlags = BGFX_RESET_VSYNC; // one frame per screen refresh

Backend backendOf(bgfx::RendererType::Enum type) {
    switch (type) {
    case bgfx::RendererType::Vulkan:
        return Backend::Vulkan;
    case bgfx::RendererType::OpenGL:
        return Backend::OpenGL;
    case bgfx::RendererType::Metal:
        return Backend::Metal;
    case bgfx::RendererType::Direct3D11:
        return Backend::Direct3D11;
    case bgfx::RendererType::Direct3D12:
        return Backend::Direct3D12;
    default:
        return Backend::Other;
    }
}

bgfx::RendererType::Enum bgfxTypeOf(Backend backend) {
    switch (backend) {
    case Backend::Vulkan:
        return bgfx::RendererType::Vulkan;
    case Backend::OpenGL:
        return bgfx::RendererType::OpenGL;
    case Backend::Metal:
        return bgfx::RendererType::Metal;
    case Backend::Direct3D11:
        return bgfx::RendererType::Direct3D11;
    case Backend::Direct3D12:
        return bgfx::RendererType::Direct3D12;
    case Backend::Other:
        break;
    }
    return bgfx::RendererType::Count; // let bgfx choose
}

// bgfx's 0xRRGGBBAA form of a colour.
std::uint32_t packed(Colour colour) {
    return (std::uint32_t{colour.red} << 24U) | (std::uint32_t{colour.green} << 16U) |
           (std::uint32_t{colour.blue} << 8U) | std::uint32_t{colour.alpha};
}

} // namespace

std::vector<Backend> BgfxRenderer::supportedBackends() {
    std::array<bgfx::RendererType::Enum, bgfx::RendererType::Count> types{};
    const std::uint8_t count =
        bgfx::getSupportedRenderers(static_cast<std::uint8_t>(types.size()), types.data());
    std::vector<Backend> backends;
    for (std::size_t i = 0; i < count; ++i) {
        const Backend backend = backendOf(types.at(i));
#ifndef _WIN32
        // bgfx also builds Direct3D outside Windows (for layers such as DXVK) and reports it as
        // supported, although it cannot start on a normal Linux desktop.
        if (backend == Backend::Direct3D11 || backend == Backend::Direct3D12) {
            continue;
        }
#endif
        if (backend != Backend::Other && std::ranges::find(backends, backend) == backends.end()) {
            backends.push_back(backend);
        }
    }
    return backends;
}

BgfxRenderer::BgfxRenderer(const platform::Window& window, std::optional<Backend> only) {
    const platform::NativeWindow native = window.nativeWindow();
    if (native.window == nullptr) {
        // bgfx would quietly start with no window at all, so stop before touching it.
        throw StartupError{"the window has no native window handle for the renderer"};
    }

    // Calling renderFrame() before init() makes bgfx render on this thread, with no render
    // thread of its own (needed on macOS, where drawing must happen on the main thread).
    bgfx::renderFrame();

    const Size size = window.pixelSize();
    bgfx::Init init;
    // The chosen backend and nothing else, or the best one for this computer with fallback.
    const bool chosen = only.has_value() && *only != Backend::Other;
    init.type = chosen ? bgfxTypeOf(*only) : bgfx::RendererType::Count;
    init.fallback = !chosen;
    init.platformData.type = native.kind == platform::NativeWindow::Kind::Wayland
                                 ? bgfx::NativeWindowHandleType::Wayland
                                 : bgfx::NativeWindowHandleType::Default;
    init.swapChain.nwh = native.window;
    init.swapChain.ndt = native.display;
    init.swapChain.width = static_cast<std::uint32_t>(size.width);
    init.swapChain.height = static_cast<std::uint32_t>(size.height);
    init.reset = resetFlags;
    if (!bgfx::init(init)) {
        throw StartupError{"no renderer could start"};
    }

    // From here bgfx is running: if anything below fails, the destructor will not run, so shut
    // bgfx down here before the window it draws into goes away.
    try {
        const bgfx::RendererType::Enum type = bgfx::getRendererType();
        name_ = RendererName::display(backendOf(type), bgfx::getRendererName(type));
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
    } catch (...) {
        bgfx::shutdown();
        throw;
    }
}

BgfxRenderer::~BgfxRenderer() {
    bgfx::shutdown();
}

std::string_view BgfxRenderer::name() const {
    return name_;
}

void BgfxRenderer::resize(Size pixelSize) {
    bgfx::SwapChain swapChain;
    swapChain.width = static_cast<std::uint32_t>(pixelSize.width);
    swapChain.height = static_cast<std::uint32_t>(pixelSize.height);
    bgfx::reset(resetFlags, &swapChain);
}

void BgfxRenderer::drawFrame(Colour background) {
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR, packed(background));
    bgfx::touch(mainView); // the view is drawn (cleared) even with nothing submitted to it
    bgfx::frame();
}

} // namespace wolfen::engine::render
