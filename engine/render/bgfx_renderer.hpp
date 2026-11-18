#ifndef WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP
#define WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP

#include "platform/window.hpp"
#include "render/renderer.hpp"

#include <string>

namespace wolfen::engine::render {

// A renderer built on bgfx, drawing into one window on the calling thread. bgfx is a single
// global system, so only one BgfxRenderer may exist at a time.
class BgfxRenderer final : public Renderer {
  public:
    // Lets bgfx pick the best backend for this computer. Throws StartupError when none starts.
    explicit BgfxRenderer(const platform::Window& window);
    BgfxRenderer(const BgfxRenderer&) = delete;
    BgfxRenderer& operator=(const BgfxRenderer&) = delete;
    BgfxRenderer(BgfxRenderer&&) = delete;
    BgfxRenderer& operator=(BgfxRenderer&&) = delete;
    ~BgfxRenderer() override;

    [[nodiscard]] std::string_view name() const override;
    void resize(Size pixelSize) override;
    void drawFrame(Colour background) override;

  private:
    std::string name_;
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP
