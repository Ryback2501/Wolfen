#include "core/colour.hpp"
#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "game.hpp"
#include "platform/window.hpp"
#include "platform/window_mode.hpp"
#include "render/graphics_api_name.hpp"
#include "runs.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

using wolfen::engine::Colour;
using wolfen::engine::Size;
using wolfen::engine::Version;
using wolfen::engine::platform::Window;
using wolfen::engine::platform::WindowMode;
using wolfen::engine::render::GraphicsApi;
using wolfen::engine::render::GraphicsApiName;
using wolfen::game::Game;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::GameRun;
using wolfen::tests::versionLine;

TEST_CASE("The start line is printed before the window opens", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.outputWhenWindowOpened == versionLine());
    REQUIRE(run.out() == versionLine());
}

TEST_CASE("The window opens 1280x720, resizable, titled with the version", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    const GameRun run{graphicsSystem};
    REQUIRE_FALSE(graphicsSystem.record.steps.empty());
    REQUIRE(graphicsSystem.record.steps.front() == "open window");
    const auto& windowSettings = graphicsSystem.record.windowSettings;
    REQUIRE(windowSettings.windowedSize == Size{.width = 1280, .height = 720});
    REQUIRE(windowSettings.isResizable);
    REQUIRE(windowSettings.title == "Wolfen " + std::string{Version::current().text()});
}

TEST_CASE("The title shows the version and the renderer in use", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererDisplayName = "Vulkan";
    const GameRun run{graphicsSystem};
    REQUIRE_FALSE(graphicsSystem.record.titles.empty());
    REQUIRE(graphicsSystem.record.titles.back() ==
            "Wolfen " + std::string{Version::current().text()} + " (Vulkan)");
}

TEST_CASE("Every frame fills the window with cornflower blue", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{}, {}, {}, {Window::Event::CloseRequested}};
    const GameRun run{graphicsSystem};
    const Colour cornflowerBlue{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};
    REQUIRE(graphicsSystem.record.frameColours == std::vector<Colour>(3, cornflowerBlue));
}

TEST_CASE("The close button ends the game", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{}, {Window::Event::CloseRequested}, {}};
    const GameRun run{graphicsSystem};
    REQUIRE(run.outcome() == Game::Outcome::Ended);
    REQUIRE(graphicsSystem.record.frameColours.size() == 1);
    REQUIRE(run.err().empty());
}

TEST_CASE("Esc ends the game", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{}, {Window::Event::EscapePressed}, {}};
    const GameRun run{graphicsSystem};
    REQUIRE(run.outcome() == Game::Outcome::Ended);
    REQUIRE(graphicsSystem.record.frameColours.size() == 1);
}

TEST_CASE("Resizing the window gives the renderer the new size in pixels", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.pixelSize = Size{.width = 1280, .height = 720};
    graphicsSystem.sizeAfterResize = Size{.width = 1600, .height = 900};
    graphicsSystem.events = {{}, {Window::Event::Resized}, {}};
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.resizedPixelSizes ==
            std::vector<Size>{Size{.width = 1600, .height = 900}});
}

TEST_CASE("When the game ends, the renderer closes before the window", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.steps == std::vector<std::string>{"open window", "start renderer",
                                                                    "close renderer",
                                                                    "close window"});
}

TEST_CASE("A window that cannot open: message on stderr, the game could not start",
          "[game][startup]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.windowFailure = "no display available";
    const GameRun run{graphicsSystem};
    REQUIRE(run.outcome() == Game::Outcome::CouldNotStart);
    REQUIRE(run.err() == "Wolfen could not start: no display available\n");
    REQUIRE(graphicsSystem.record.steps == std::vector<std::string>{"open window"});
}

TEST_CASE("A renderer that cannot start: message on stderr, the game could not start, window "
          "closed",
          "[game][startup]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererFailure = "no renderer could start";
    const GameRun run{graphicsSystem};
    REQUIRE(run.outcome() == Game::Outcome::CouldNotStart);
    REQUIRE(run.err() == "Wolfen could not start: no renderer could start\n");
    REQUIRE(graphicsSystem.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
    REQUIRE(graphicsSystem.record.frameColours.empty());
}

TEST_CASE("Renderer display names", "[renderer]") {
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::Vulkan, "Vulkan") == "Vulkan");
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::OpenGL, "OpenGL 2.1") == "OpenGL");
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::Metal, "Metal") == "Metal");
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::Direct3D11, "Direct3D 11") == "Direct3D 11");
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::Direct3D12, "Direct3D 12") == "Direct3D 12");
}

TEST_CASE("Any other renderer shows the rendering library's own name", "[renderer]") {
    REQUIRE(GraphicsApiName::displayName(GraphicsApi::Other, "Noop") == "Noop");
}

TEST_CASE("A hidden window is not drawn; the game sleeps until it is shown again", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{}, {Window::Event::Hidden}, {}, {}, {Window::Event::Shown}, {}, {}};
    const GameRun run{graphicsSystem};
    // Drawn after the 1st batch; not after Hidden (2nd) nor the batches waited for while hidden
    // (3rd, 4th); again after Shown (5th), the 6th and the 7th. The 8th poll finds none left and
    // closes.
    REQUIRE(graphicsSystem.record.frameColours.size() == 4);
    // The batches after Hidden (3rd, 4th, 5th) are waited for, not polled.
    REQUIRE(graphicsSystem.record.waitCount == 3);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("A hidden window can still be closed", "[game]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{Window::Event::Hidden}, {Window::Event::CloseRequested}};
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.frameColours.empty());
    REQUIRE(graphicsSystem.record.waitCount == 1);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("The game starts the renderer it is given, after the start line", "[game][renderer]") {
    for (const auto graphicsApi : {GraphicsApi::Vulkan, GraphicsApi::OpenGL}) {
        FakeGraphicsSystem graphicsSystem;
        const GameRun run{graphicsSystem, graphicsApi};
        REQUIRE(graphicsSystem.record.outputWhenWindowOpened == versionLine());
        REQUIRE(graphicsSystem.record.requestedGraphicsApi == graphicsApi);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}

TEST_CASE("Given no renderer, the game lets the engine choose", "[game][renderer]") {
    FakeGraphicsSystem graphicsSystem;
    const GameRun run{graphicsSystem, std::nullopt};
    REQUIRE(graphicsSystem.record.steps.size() >= 2);
    REQUIRE(graphicsSystem.record.steps.at(1) == "start renderer");
    REQUIRE_FALSE(graphicsSystem.record.requestedGraphicsApi.has_value());
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("A given renderer that fails to start: reported, nothing printed, window closed",
          "[game][renderer][startup]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererFailure = "the device could not be created";
    const GameRun run{graphicsSystem, GraphicsApi::Vulkan};
    REQUIRE(run.outcome() == Game::Outcome::ChosenGraphicsApiFailed);
    REQUIRE(run.out() == versionLine()); // the start line comes before the failure
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
}

TEST_CASE("Given no window mode, the game opens a window, not fullscreen", "[game][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.windowSettings.windowMode == WindowMode::Windowed);
}

TEST_CASE("Asked for fullscreen, the game opens its window fullscreen, 1280x720 for later",
          "[game][fullscreen]") {
    for (const auto graphicsApi :
         {std::optional<GraphicsApi>{}, std::optional<GraphicsApi>{GraphicsApi::OpenGL}}) {
        FakeGraphicsSystem graphicsSystem;
        const GameRun run{graphicsSystem, graphicsApi, WindowMode::Fullscreen};
        const auto& windowSettings = graphicsSystem.record.windowSettings;
        REQUIRE(windowSettings.windowMode == WindowMode::Fullscreen);
        // The size the window gets when it first leaves fullscreen.
        REQUIRE(windowSettings.windowedSize == Size{.width = 1280, .height = 720});
        REQUIRE(graphicsSystem.record.requestedGraphicsApi == graphicsApi);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}

TEST_CASE("Each F11 press switches fullscreen once, and the game keeps drawing",
          "[game][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {
        {Window::Event::FullscreenKeyPressed}, {}, {Window::Event::FullscreenKeyPressed}, {}};
    const GameRun run{graphicsSystem};
    REQUIRE(graphicsSystem.record.fullscreenSwitchCount == 2);
    REQUIRE(graphicsSystem.record.frameColours.size() == 4);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("Switching fullscreen keeps the title with the renderer's name", "[game][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererDisplayName = "OpenGL";
    graphicsSystem.events = {{Window::Event::FullscreenKeyPressed}, {}};
    const GameRun run{graphicsSystem, GraphicsApi::OpenGL, WindowMode::Fullscreen};
    REQUIRE(graphicsSystem.record.fullscreenSwitchCount == 1);
    REQUIRE(
        graphicsSystem.record.titles ==
        std::vector<std::string>{"Wolfen " + std::string{Version::current().text()} + " (OpenGL)"});
}

TEST_CASE("In fullscreen, Esc and the close button still end the game", "[game][fullscreen]") {
    for (const auto quit : {Window::Event::EscapePressed, Window::Event::CloseRequested}) {
        FakeGraphicsSystem graphicsSystem;
        graphicsSystem.events = {{}, {quit}, {}};
        const GameRun run{graphicsSystem, std::nullopt, WindowMode::Fullscreen};
        REQUIRE(graphicsSystem.record.windowSettings.windowMode == WindowMode::Fullscreen);
        REQUIRE(graphicsSystem.record.frameColours.size() == 1);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}
