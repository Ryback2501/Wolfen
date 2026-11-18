#include "fake_graphics.hpp"
#include "render/bgfx_renderer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;
using wolfen::engine::Size;
using wolfen::engine::render::BgfxRenderer;
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
