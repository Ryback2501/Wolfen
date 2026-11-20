#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "launcher.hpp"
#include "render/renderer_name.hpp"
#include "runs.hpp"

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <string>
#include <string_view>

using wolfen::engine::Version;
using wolfen::engine::render::Backend;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::LauncherRun;

namespace {

std::string versionLine() {
    return "Wolfen " + std::string{Version::current().text()} + "\n";
}

// The usage text approved for #3, with the given list of available renderers.
std::string usage(std::string_view available) {
    return "Usage: wolfen [options]\n"
           "\n"
           "Options:\n"
           "  --renderer <name>  Draw with this renderer. Available on this computer: " +
           std::string{available} +
           "\n"
           "  --help             Show this help and exit.\n"
           "  --version          Show the version and exit.\n"
           "\n"
           "While the game runs, press Esc to quit.\n";
}

const std::string usageHere = usage("vulkan, opengl"); // the fake's default available renderers

// The one line shown after an option error, instead of the usage.
const std::string hintLine = "Run 'wolfen --help' to see the available options.\n";

// An option error: the message on stderr, then the hint line; nothing on stdout, no window, exit 1.
void requireOptionError(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args,
                        const std::string& message) {
    const LauncherRun run{graphics, args};
    REQUIRE(run.err() == message + "\n" + hintLine);
    REQUIRE(run.out().empty());
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(run.exitCode() == 1);
}

} // namespace

TEST_CASE("The --help option prints the usage, opens no window and exits with 0",
          "[options][help]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, {"--help"}};
    REQUIRE(run.out() == usageHere); // the usage only: no start line
    REQUIRE(run.err().empty());
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --help option wins over a wrong option and over --version", "[options][help]") {
    for (const auto args : {std::initializer_list<std::string_view>{"--bogus", "--help"},
                            std::initializer_list<std::string_view>{"--version", "--help"},
                            std::initializer_list<std::string_view>{"--renderer", "--help"}}) {
        FakeGraphicsSystem graphics;
        const LauncherRun run{graphics, args};
        REQUIRE(run.out() == usageHere);
        REQUIRE(run.err().empty());
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("The usage lists only the renderers available on this computer, in a fixed order",
          "[options][help]") {
    FakeGraphicsSystem graphics;
    graphics.available = {Backend::OpenGL, Backend::Vulkan, Backend::Direct3D12};
    REQUIRE(LauncherRun{graphics, {"--help"}}.out() == usage("vulkan, opengl, d3d12"));

    FakeGraphicsSystem mac;
    mac.available = {Backend::Metal};
    REQUIRE(LauncherRun{mac, {"--help"}}.out() == usage("metal"));

    FakeGraphicsSystem none;
    none.available = {};
    REQUIRE(LauncherRun{none, {"--help"}}.out() == usage("(none)"));
}

TEST_CASE("The --version option prints exactly the version line, opens no window and exits with 0",
          "[options][version]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, {"--version"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --version option wins over wrong and repeated options", "[options][version]") {
    for (const auto args :
         {std::initializer_list<std::string_view>{"--version", "--bogus"},
          std::initializer_list<std::string_view>{"--renderer", "foo", "--version"},
          std::initializer_list<std::string_view>{"--version", "--version"},
          std::initializer_list<std::string_view>{"--renderer", "--version"}}) {
        FakeGraphicsSystem graphics;
        const LauncherRun run{graphics, args};
        REQUIRE(run.out() == versionLine());
        REQUIRE(run.err().empty());
        REQUIRE(graphics.record.steps.empty());
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("The --renderer option starts only the chosen renderer, after the start line",
          "[options][renderer]") {
    for (const auto& [name, backend] :
         {std::pair{"vulkan", Backend::Vulkan}, std::pair{"opengl", Backend::OpenGL}}) {
        FakeGraphicsSystem graphics;
        const LauncherRun run{graphics, {"--renderer", name}};
        REQUIRE(run.out() == versionLine());
        REQUIRE(graphics.record.requestedRenderer == backend);
        REQUIRE(graphics.record.steps.front() == "open window");
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("With no options the game lets the engine choose the renderer", "[options][renderer]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, {}};
    REQUIRE_FALSE(graphics.record.requestedRenderer.has_value());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("A renderer that is not available on this computer", "[options][renderer]") {
    FakeGraphicsSystem graphics;
    requireOptionError(graphics, {"--renderer", "metal"},
                       "Renderer 'metal' is not available on this computer. Available: vulkan, "
                       "opengl");
}

TEST_CASE("A renderer name Wolfen does not know", "[options][renderer]") {
    for (const auto name : {std::string_view{"foo"}, std::string_view{"Vulkan"}}) {
        FakeGraphicsSystem graphics;
        requireOptionError(graphics, {"--renderer", name},
                           "Unknown renderer '" + std::string{name} +
                               "'. Available: vulkan, opengl");
    }
}

TEST_CASE("The --renderer option with no name", "[options][renderer]") {
    FakeGraphicsSystem alone;
    requireOptionError(alone, {"--renderer"}, "Option '--renderer' needs a renderer name");
    // A word starting with '-' is never a name: the first error is the missing name.
    FakeGraphicsSystem followed;
    requireOptionError(followed, {"--renderer", "--bogus"},
                       "Option '--renderer' needs a renderer name");
}

TEST_CASE("The --renderer option given twice", "[options][renderer]") {
    FakeGraphicsSystem graphics;
    requireOptionError(graphics, {"--renderer", "opengl", "--renderer", "vulkan"},
                       "Option '--renderer' given twice");
}

TEST_CASE("Unknown options, including the --option=value form", "[options]") {
    for (const auto word : {std::string_view{"--bogus"}, std::string_view{"foo"},
                            std::string_view{"--renderer=vulkan"}}) {
        FakeGraphicsSystem graphics;
        requireOptionError(graphics, {word}, "Unknown option '" + std::string{word} + "'");
    }
}

TEST_CASE("Only the first error, reading left to right, is printed", "[options]") {
    FakeGraphicsSystem graphics;
    requireOptionError(graphics, {"--bogus", "--renderer", "foo"}, "Unknown option '--bogus'");
}
