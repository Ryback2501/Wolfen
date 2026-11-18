#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "game.hpp"
#include "render/renderer_name.hpp"

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using wolfen::engine::Version;
using wolfen::engine::render::Backend;
using wolfen::game::Game;
using wolfen::tests::FakeGraphicsSystem;

namespace {

// Runs the game once with the given start options and fake graphics system.
class GameRun {
  public:
    GameRun(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args)
        : args_{args} {
        Game game{out_, err_, graphics};
        exitCode_ = game.run(args_);
    }

    [[nodiscard]] int exitCode() const {
        return exitCode_;
    }
    [[nodiscard]] std::string out() const {
        return out_.str();
    }
    [[nodiscard]] std::string err() const {
        return err_.str();
    }

  private:
    std::vector<std::string_view> args_;
    std::ostringstream out_;
    std::ostringstream err_;
    int exitCode_{-1};
};

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

// An option error: the message on stderr, then the usage; nothing on stdout, no window, exit 1.
void requireOptionError(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args,
                        const std::string& message) {
    const GameRun run{graphics, args};
    REQUIRE(run.err() == message + "\n" + usageHere);
    REQUIRE(run.out().empty());
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(run.exitCode() == 1);
}

} // namespace

TEST_CASE("The --help option prints the usage, opens no window and exits with 0",
          "[options][help]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics, {"--help"}};
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
        const GameRun run{graphics, args};
        REQUIRE(run.out() == usageHere);
        REQUIRE(run.err().empty());
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("The usage lists only the renderers available on this computer, in a fixed order",
          "[options][help]") {
    FakeGraphicsSystem graphics;
    graphics.available = {Backend::OpenGL, Backend::Vulkan, Backend::Direct3D12};
    REQUIRE(GameRun{graphics, {"--help"}}.out() == usage("vulkan, opengl, d3d12"));

    FakeGraphicsSystem mac;
    mac.available = {Backend::Metal};
    REQUIRE(GameRun{mac, {"--help"}}.out() == usage("metal"));

    FakeGraphicsSystem none;
    none.available = {};
    REQUIRE(GameRun{none, {"--help"}}.out() == usage("(none)"));
}

TEST_CASE("The --version option prints exactly the version line, opens no window and exits with 0",
          "[options][version]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics, {"--version"}};
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
        const GameRun run{graphics, args};
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
        const GameRun run{graphics, {"--renderer", name}};
        REQUIRE(run.out() == versionLine());
        REQUIRE(graphics.record.requestedRenderer == backend);
        REQUIRE(graphics.record.steps.front() == "open window");
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("With no options the game lets the engine choose the renderer", "[options][renderer]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics, {}};
    REQUIRE(graphics.record.steps.size() >= 2);
    REQUIRE(graphics.record.steps.at(1) == "start renderer");
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

TEST_CASE("A chosen renderer that fails to start: its own message, exit 1, window closed",
          "[options][renderer][startup]") {
    FakeGraphicsSystem graphics;
    graphics.rendererFailure = "the device could not be created";
    const GameRun run{graphics, {"--renderer", "vulkan"}};
    REQUIRE(run.out() == versionLine()); // the start line comes before the failure
    REQUIRE(run.err() == "Renderer 'vulkan' could not be started on this computer.\n");
    REQUIRE(graphics.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
    REQUIRE(run.exitCode() == 1);
}

TEST_CASE("Other start failures keep the 'Wolfen could not start' prefix and exit with 1",
          "[options][startup]") {
    FakeGraphicsSystem graphics;
    graphics.windowFailure = "no display available";
    const GameRun run{graphics, {}};
    REQUIRE(run.err() == "Wolfen could not start: no display available\n");
    REQUIRE(run.exitCode() == 1);
}
