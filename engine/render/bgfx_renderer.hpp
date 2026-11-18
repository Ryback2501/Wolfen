#ifndef WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP
#define WOLFEN_ENGINE_RENDER_BGFX_RENDERER_HPP

#include "platform/window.hpp"
#include "render/renderer.hpp"
#include "render/renderer_name.hpp"

#include <optional>
#include <string>
#include <vector>

namespace wolfen::engine::render {

// A renderer built on bgfx, drawing into one window on the calling thread. bgfx is a single
// global system, so only one BgfxRenderer may exist at a time.
class BgfxRenderer final : public Renderer {
  public:
    // The backends bgfx supports on this computer, among the ones Wolfen names. Needs no window
    // and does not start bgfx.
    static std::vector<Backend> supportedBackends();

    // Starts the given backend and no other one; without one, lets bgfx pick the best backend
    // for this computer, falling back to another if it fails. Throws StartupError when it cannot.
    explicit BgfxRenderer(const platform::Window& window,
                          std::optional<Backend> only = std::nullopt);
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
