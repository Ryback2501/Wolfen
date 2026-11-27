#include "render/bgfx_renderer.hpp"

#include "core/startup_error.hpp"
#include "render/graphics_api_name.hpp"
#include "render/graphics_api_table.hpp"

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

// The graphics API for a bgfx renderer type; GraphicsApi::Other for a type Wolfen does not name.
GraphicsApi graphicsApiOf(bgfx::RendererType::Enum type) {
    const GraphicsApiTable::Row* row = GraphicsApiTable::row(type);
    return row == nullptr ? GraphicsApi::Other : row->graphicsApi;
}

// The bgfx renderer type for a graphics API; for GraphicsApi::Other, "let bgfx choose".
bgfx::RendererType::Enum bgfxTypeOf(GraphicsApi graphicsApi) {
    const GraphicsApiTable::Row* row = GraphicsApiTable::row(graphicsApi);
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

// Windows' software rasterizer (WARP) as Direct3D lists it: the "Microsoft Basic Render Driver",
// with Microsoft's vendor id.
constexpr std::uint16_t warpDeviceId = 0x008C;

// Whether bgfx draws on a CPU device.
// - Direct3D: on the WARP adapter, or on an adapter with no vendor id, which bgfx reports with its
//   software id.
// - Vulkan: bgfx keeps the device's own vendor id, but its list of devices marks CPU devices with
//   its software id. The device in use is a CPU one when the list has a software entry with its
//   device id, and no graphics card with both its vendor and device ids.
// WARP that bgfx falls back to by itself (Direct3D 11 retrying on a graphics card's adapter) is
// not recognised.
bool drawsOnCpu(const bgfx::Caps& caps) {
    if (caps.vendorId == BGFX_PCI_ID_SOFTWARE_RASTERIZER ||
        (caps.vendorId == BGFX_PCI_ID_MICROSOFT && caps.deviceId == warpDeviceId)) {
        return true;
    }
    const std::span<const bgfx::Caps::GPU> gpus{caps.gpu};
    const auto listed = gpus.first(std::min<std::size_t>(caps.numGPUs, gpus.size()));
    const bool isListedAsCpu = std::ranges::any_of(listed, [&caps](const bgfx::Caps::GPU& gpu) {
        return gpu.vendorId == BGFX_PCI_ID_SOFTWARE_RASTERIZER && gpu.deviceId == caps.deviceId;
    });
    const bool isListedAsGraphicsCard =
        std::ranges::any_of(listed, [&caps](const bgfx::Caps::GPU& gpu) {
            return gpu.vendorId == caps.vendorId && gpu.deviceId == caps.deviceId;
        });
    return isListedAsCpu && !isListedAsGraphicsCard;
}

// bgfx's 0xRRGGBBAA form of a colour.
std::uint32_t packed(Colour colour) {
    return (std::uint32_t{colour.red} << 24U) | (std::uint32_t{colour.green} << 16U) |
           (std::uint32_t{colour.blue} << 8U) | std::uint32_t{colour.alpha};
}

} // namespace

std::vector<GraphicsApi> BgfxRenderer::availableGraphicsApis() {
    std::array<bgfx::RendererType::Enum, bgfx::RendererType::Count> types{};
    const std::uint8_t count =
        bgfx::getSupportedRenderers(static_cast<std::uint8_t>(types.size()), types.data());
    std::vector<GraphicsApi> graphicsApis;
    for (std::size_t i = 0; i < count; ++i) {
        const GraphicsApi graphicsApi = graphicsApiOf(types.at(i));
#ifndef _WIN32
        // bgfx also builds Direct3D outside Windows (for layers such as DXVK) and reports it as
        // supported, although it cannot start on a normal Linux desktop.
        if (graphicsApi == GraphicsApi::Direct3D11 || graphicsApi == GraphicsApi::Direct3D12) {
            continue;
        }
#endif
        if (graphicsApi != GraphicsApi::Other &&
            std::ranges::find(graphicsApis, graphicsApi) == graphicsApis.end()) {
            graphicsApis.push_back(graphicsApi);
        }
    }
    return graphicsApis;
}

BgfxRenderer::BgfxRenderer(const platform::Window& window, std::optional<GraphicsApi> graphicsApi,
                           Device device) {
    const platform::Window::NativeHandles nativeHandles = window.nativeHandles();
    if (nativeHandles.window == nullptr) {
        // bgfx would quietly start with no window at all, so stop before touching it.
        throw StartupError{"the window has no native window handle for the renderer"};
    }

    // Calling renderFrame() before init() makes bgfx render on this thread, with no render
    // thread of its own (needed on macOS, where drawing must happen on the main thread).
    bgfx::renderFrame();

    bgfx::Init init;
    // The chosen graphics API and nothing else, or the best one for this computer with fallback.
    const bool hasChosenGraphicsApi = graphicsApi.has_value() && *graphicsApi != GraphicsApi::Other;
    init.type = hasChosenGraphicsApi ? bgfxTypeOf(*graphicsApi) : bgfx::RendererType::Count;
    init.fallback = !hasChosenGraphicsApi;
    if (device == Device::Cpu) {
        // Direct3D picks the adapter with these ids: WARP's, as other Microsoft adapters (a virtual
        // machine's or a remote session's display) may come first. bgfx looks only at the first
        // four adapters and WARP is listed last, so a computer with more misses it; this is
        // accepted. Vulkan picks the device bgfx marks with its software id: a device of type CPU.
        if (graphicsApi == GraphicsApi::Direct3D11 || graphicsApi == GraphicsApi::Direct3D12) {
            init.vendorId = BGFX_PCI_ID_MICROSOFT;
            init.deviceId = warpDeviceId;
        } else {
            init.vendorId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
        }
    }
    init.platformData.type = nativeHandles.kind == platform::Window::NativeHandles::Kind::Wayland
                                 ? bgfx::NativeWindowHandleType::Wayland
                                 : bgfx::NativeWindowHandleType::Default;
    init.swapChain.nwh = nativeHandles.window;
    init.swapChain.ndt = nativeHandles.display;
    setSwapChainSize(init.swapChain, window.pixelSize());
    init.reset = resetFlags;
    if (!bgfx::init(init)) {
        throw StartupError{"no renderer could start"};
    }

    // From here bgfx is running: if anything below fails, the destructor will not run, so shut
    // bgfx down here before the window it draws into goes away.
    try {
        const bgfx::RendererType::Enum type = bgfx::getRendererType();
        displayName_ =
            GraphicsApiName::displayName(graphicsApiOf(type), bgfx::getRendererName(type));
        drawsWithCpu_ = drawsOnCpu(*bgfx::getCaps());
        if (device == Device::Cpu && !drawsWithCpu_) {
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

std::string_view BgfxRenderer::displayName() const {
    return displayName_;
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

void BgfxRenderer::drawFrame(Colour backgroundColour) {
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR, packed(backgroundColour));
    bgfx::touch(mainView); // the view is drawn (cleared) even with nothing submitted to it
    bgfx::frame();
}

} // namespace wolfen::engine::render
