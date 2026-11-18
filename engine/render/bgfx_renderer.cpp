#include "render/bgfx_renderer.hpp"

#include "core/startup_error.hpp"
#include "render/renderer_name.hpp"

#include <bgfx/bgfx.h>

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

// bgfx's 0xRRGGBBAA form of a colour.
std::uint32_t packed(Colour colour) {
    return (std::uint32_t{colour.red} << 24U) | (std::uint32_t{colour.green} << 16U) |
           (std::uint32_t{colour.blue} << 8U) | std::uint32_t{colour.alpha};
}

} // namespace

BgfxRenderer::BgfxRenderer(const platform::Window& window) {
    // Calling renderFrame() before init() makes bgfx render on this thread, with no render
    // thread of its own (needed on macOS, where drawing must happen on the main thread).
    bgfx::renderFrame();

    const platform::NativeWindow native = window.nativeWindow();
    const Size size = window.pixelSize();
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // the best backend for this computer
    init.fallback = true;
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

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    name_ = RendererName::display(backendOf(type), bgfx::getRendererName(type));
    bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
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
