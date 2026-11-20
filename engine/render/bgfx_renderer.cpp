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

// Every backend Wolfen names, next to bgfx's renderer type for it. Each pair is written once.
struct BackendType {
    Backend backend;
    bgfx::RendererType::Enum type;
};

constexpr std::array<BackendType, 5> backendTypes{
    {{.backend = Backend::Vulkan, .type = bgfx::RendererType::Vulkan},
     {.backend = Backend::OpenGL, .type = bgfx::RendererType::OpenGL},
     {.backend = Backend::Metal, .type = bgfx::RendererType::Metal},
     {.backend = Backend::Direct3D11, .type = bgfx::RendererType::Direct3D11},
     {.backend = Backend::Direct3D12, .type = bgfx::RendererType::Direct3D12}}};

// The backend for a bgfx renderer type; Backend::Other for a type not in the table.
Backend backendOf(bgfx::RendererType::Enum type) {
    for (const BackendType& pair : backendTypes) {
        if (pair.type == type) {
            return pair.backend;
        }
    }
    return Backend::Other;
}

// The bgfx renderer type for a backend; for Backend::Other, "let bgfx choose".
bgfx::RendererType::Enum bgfxTypeOf(Backend backend) {
    for (const BackendType& pair : backendTypes) {
        if (pair.backend == backend) {
            return pair.type;
        }
    }
    return bgfx::RendererType::Count; // let bgfx choose
}

// Sets the swap chain's width and height to a size in pixels.
void setSwapChainSize(bgfx::SwapChain& swapChain, Size pixelSize) {
    swapChain.width = static_cast<std::uint32_t>(pixelSize.width);
    swapChain.height = static_cast<std::uint32_t>(pixelSize.height);
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

BgfxRenderer::BgfxRenderer(const platform::Window& window, std::optional<Backend> backend) {
    const platform::NativeWindow nativeWindow = window.nativeWindow();
    if (nativeWindow.window == nullptr) {
        // bgfx would quietly start with no window at all, so stop before touching it.
        throw StartupError{"the window has no native window handle for the renderer"};
    }

    // Calling renderFrame() before init() makes bgfx render on this thread, with no render
    // thread of its own (needed on macOS, where drawing must happen on the main thread).
    bgfx::renderFrame();

    bgfx::Init init;
    // The chosen backend and nothing else, or the best one for this computer with fallback.
    const bool backendChosen = backend.has_value() && *backend != Backend::Other;
    init.type = backendChosen ? bgfxTypeOf(*backend) : bgfx::RendererType::Count;
    init.fallback = !backendChosen;
    init.platformData.type = nativeWindow.kind == platform::NativeWindow::Kind::Wayland
                                 ? bgfx::NativeWindowHandleType::Wayland
                                 : bgfx::NativeWindowHandleType::Default;
    init.swapChain.nwh = nativeWindow.window;
    init.swapChain.ndt = nativeWindow.display;
    setSwapChainSize(init.swapChain, window.pixelSize());
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
    setSwapChainSize(swapChain, pixelSize);
    bgfx::reset(resetFlags, &swapChain);
}

void BgfxRenderer::drawFrame(Colour background) {
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR, packed(background));
    bgfx::touch(mainView); // the view is drawn (cleared) even with nothing submitted to it
    bgfx::frame();
}

} // namespace wolfen::engine::render
