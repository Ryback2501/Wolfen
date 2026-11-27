#include "fake_graphics.hpp"
#include "render/bgfx_renderer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using wolfen::engine::Size;
using wolfen::engine::render::BgfxRenderer;
using wolfen::engine::render::GraphicsApi;
using wolfen::tests::FakeWindow;
using wolfen::tests::GraphicsRecord;

TEST_CASE("The renderer refuses a window without a native handle, before starting bgfx",
          "[renderer][startup]") {
    // A window on an unexpected video driver hands out no handle; without this check bgfx would
    // quietly start with no window at all.
    GraphicsRecord record;
    const FakeWindow window{
        record, {}, Size{.width = 1280, .height = 720}, Size{.width = 1280, .height = 720}};
    REQUIRE_THROWS_WITH(BgfxRenderer{window}, ContainsSubstring("no native window handle"));
}

TEST_CASE("Outside Windows, Direct3D is never reported as available", "[renderer]") {
    // Only asks bgfx what it supports: no window, no GPU, bgfx is not started.
    const std::vector<GraphicsApi> backends = BgfxRenderer::availableBackends();
#ifdef _WIN32
    SUCCEED("Direct3D belongs on Windows");
#else
    REQUIRE(std::ranges::find(backends, GraphicsApi::Direct3D11) == backends.end());
    REQUIRE(std::ranges::find(backends, GraphicsApi::Direct3D12) == backends.end());
#endif
}

TEST_CASE("bgfx reports at least one renderer Wolfen can name", "[renderer]") {
    // Every platform Wolfen builds for has at least Vulkan, OpenGL, Metal or Direct3D compiled in.
    REQUIRE_FALSE(BgfxRenderer::availableBackends().empty());
}
