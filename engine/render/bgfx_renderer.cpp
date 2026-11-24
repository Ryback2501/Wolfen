#include "render/bgfx_renderer.hpp"

#include "core/startup_error.hpp"
#include "render/renderer_name.hpp"
#include "render/renderer_table.hpp"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace wolfen::engine::render {

namespace {

constexpr bgfx::ViewId mainView = 0;
constexpr std::uint32_t resetFlags = BGFX_RESET_VSYNC; // one frame per screen refresh

// The backend for a bgfx renderer type; Backend::Other for a type Wolfen does not name.
Backend backendOf(bgfx::RendererType::Enum type) {
    const RendererRow* row = RendererTable::find(type);
    return row == nullptr ? Backend::Other : row->backend;
}

// The bgfx renderer type for a backend; for Backend::Other, "let bgfx choose".
bgfx::RendererType::Enum bgfxTypeOf(Backend backend) {
    const RendererRow* row = RendererTable::find(backend);
    return row == nullptr ? bgfx::RendererType::Count : row->bgfxType;
}

// Makes the main view cover the whole window. bgfx works out a view's size only when it is set, so
// this is needed again after every resize.
void fitMainViewToWindow() {
    bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
}

// Sets the swap chain's width and height to a size in pixels.
void setSwapChainSize(bgfx::SwapChain& swapChain, Size pixelSize) {
    swapChain.width = static_cast<std::uint32_t>(pixelSize.width);
    swapChain.height = static_cast<std::uint32_t>(pixelSize.height);
}

// Whether bgfx draws on a CPU device. Direct3D reports Windows' software rasterizer (WARP) with
// bgfx's software id or Microsoft's own vendor id, and Microsoft makes no graphics cards. Vulkan
// reports the device's own vendor id, but marks CPU devices as software rasterizers in its list of
// devices.
bool drawsOnCpu(const bgfx::Caps& caps) {
    if (caps.vendorId == BGFX_PCI_ID_SOFTWARE_RASTERIZER ||
        caps.vendorId == BGFX_PCI_ID_MICROSOFT) {
        return true;
    }
    const std::span<const bgfx::Caps::GPU> gpus{caps.gpu};
    return std::ranges::any_of(gpus.first(std::min<std::size_t>(caps.numGPUs, gpus.size())),
                               [&caps](const bgfx::Caps::GPU& gpu) {
                                   return gpu.vendorId == BGFX_PCI_ID_SOFTWARE_RASTERIZER &&
                                          gpu.deviceId == caps.deviceId;
                               });
}

// bgfx's 0xRRGGBBAA form of a colour.
std::uint32_t packed(Colour colour) {
    return (std::uint32_t{colour.red} << 24U) | (std::uint32_t{colour.green} << 16U) |
           (std::uint32_t{colour.blue} << 8U) | std::uint32_t{colour.alpha};
}

} // namespace

std::vector<Backend> BgfxRenderer::availableBackends() {
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

BgfxRenderer::BgfxRenderer(const platform::Window& window, std::optional<Backend> backend,
                           DeviceChoice device) {
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
    if (device == DeviceChoice::Cpu) {
        init.vendorId = BGFX_PCI_ID_SOFTWARE_RASTERIZER; // WARP, or a Vulkan device of type CPU
    }
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
        drawsWithCpu_ = drawsOnCpu(*bgfx::getCaps());
        if (device == DeviceChoice::Cpu && !drawsWithCpu_) {
            // bgfx falls back to a graphics card when it finds no CPU device.
            throw StartupError{"there is no CPU device for this renderer"};
        }
        fitMainViewToWindow();
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

bool BgfxRenderer::drawsWithCpu() const {
    return drawsWithCpu_;
}

void BgfxRenderer::resize(Size pixelSize) {
    bgfx::SwapChain swapChain;
    setSwapChainSize(swapChain, pixelSize);
    bgfx::reset(resetFlags, &swapChain);
    fitMainViewToWindow();
}

void BgfxRenderer::drawFrame(Colour background) {
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR, packed(background));
    bgfx::touch(mainView); // the view is drawn (cleared) even with nothing submitted to it
    bgfx::frame();
}

} // namespace wolfen::engine::render
