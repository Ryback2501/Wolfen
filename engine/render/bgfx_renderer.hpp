#ifndef WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP
#define WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP

#include "platform/window.hpp"
#include "render/graphics_api.hpp"
#include "render/renderer.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wolfen::engine::render {

// A renderer built on bgfx, drawing into one window on the calling thread. bgfx is a single
// global system, so only one BgfxRenderer may exist at a time.
class BgfxRenderer final : public Renderer {
  public:
    // Which device a renderer draws on.
    enum class Device : std::uint8_t {
        Best, // the one bgfx picks, usually the graphics card
        Cpu,  // a CPU device (a software rasterizer) and nothing else
    };

    // The graphics APIs bgfx supports on this computer, among the ones Wolfen names. Needs no
    // window and does not start bgfx.
    static std::vector<GraphicsApi> availableGraphicsApis();

    // Starts on the given graphics API and no other one; without one, lets bgfx pick the best one
    // for this computer, falling back to another if it fails. On Device::Cpu, a start on
    // anything but a CPU device counts as failed. Throws StartupError when it cannot start.
    explicit BgfxRenderer(const platform::Window& window,
                          std::optional<GraphicsApi> graphicsApi = std::nullopt,
                          Device device = Device::Best);
    BgfxRenderer(const BgfxRenderer&) = delete;
    BgfxRenderer& operator=(const BgfxRenderer&) = delete;
    BgfxRenderer(BgfxRenderer&&) = delete;
    BgfxRenderer& operator=(BgfxRenderer&&) = delete;
    ~BgfxRenderer() override;

    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] bool drawsWithCpu() const override;
    void resize(Size pixelSize) override;
    void drawFrame(Colour background) override;

  private:
    std::string name_;
    bool drawsWithCpu_{};
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP
