#include "core/colour.hpp"
#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "game.hpp"
#include "platform/window.hpp"
#include "platform/window_mode.hpp"
#include "render/renderer_name.hpp"
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
using wolfen::engine::render::Backend;
using wolfen::engine::render::RendererName;
using wolfen::game::Game;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::GameRun;
using wolfen::tests::versionLine;

TEST_CASE("The start line is printed before the window opens", "[game]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE(graphics.record.outputWhenWindowOpened == versionLine());
    REQUIRE(run.out() == versionLine());
}

TEST_CASE("The window opens 1280x720, resizable, titled with the version", "[game]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE_FALSE(graphics.record.steps.empty());
    REQUIRE(graphics.record.steps.front() == "open window");
    const auto& settings = graphics.record.windowSettings;
    REQUIRE(settings.size == Size{.width = 1280, .height = 720});
    REQUIRE(settings.resizable);
    REQUIRE(settings.title == "Wolfen " + std::string{Version::current().text()});
}

TEST_CASE("The title shows the version and the renderer in use", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.rendererName = "Vulkan";
    const GameRun run{graphics};
    REQUIRE_FALSE(graphics.record.titles.empty());
    REQUIRE(graphics.record.titles.back() ==
            "Wolfen " + std::string{Version::current().text()} + " (Vulkan)");
}

TEST_CASE("Every frame fills the window with cornflower blue", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{}, {}, {}, {Window::Event::CloseRequested}};
    const GameRun run{graphics};
    const Colour cornflowerBlue{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};
    REQUIRE(graphics.record.frames == std::vector<Colour>(3, cornflowerBlue));
}

TEST_CASE("The close button ends the game", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{}, {Window::Event::CloseRequested}, {}};
    const GameRun run{graphics};
    REQUIRE(run.outcome() == Game::Outcome::Ended);
    REQUIRE(graphics.record.frames.size() == 1);
    REQUIRE(run.err().empty());
}

TEST_CASE("Esc ends the game", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{}, {Window::Event::EscapePressed}, {}};
    const GameRun run{graphics};
    REQUIRE(run.outcome() == Game::Outcome::Ended);
    REQUIRE(graphics.record.frames.size() == 1);
}

TEST_CASE("Resizing the window gives the renderer the new size in pixels", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.pixelSize = Size{.width = 1280, .height = 720};
    graphics.sizeAfterResize = Size{.width = 1600, .height = 900};
    graphics.events = {{}, {Window::Event::Resized}, {}};
    const GameRun run{graphics};
    REQUIRE(graphics.record.resizes == std::vector<Size>{Size{.width = 1600, .height = 900}});
}

TEST_CASE("When the game ends, the renderer closes before the window", "[game]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE(graphics.record.steps == std::vector<std::string>{"open window", "start renderer",
                                                              "close renderer", "close window"});
}

TEST_CASE("A window that cannot open: message on stderr, the game could not start",
          "[game][startup]") {
    FakeGraphicsSystem graphics;
    graphics.windowFailure = "no display available";
    const GameRun run{graphics};
    REQUIRE(run.outcome() == Game::Outcome::CouldNotStart);
    REQUIRE(run.err() == "Wolfen could not start: no display available\n");
    REQUIRE(graphics.record.steps == std::vector<std::string>{"open window"});
}

TEST_CASE("A renderer that cannot start: message on stderr, the game could not start, window "
          "closed",
          "[game][startup]") {
    FakeGraphicsSystem graphics;
    graphics.rendererFailure = "no renderer could start";
    const GameRun run{graphics};
    REQUIRE(run.outcome() == Game::Outcome::CouldNotStart);
    REQUIRE(run.err() == "Wolfen could not start: no renderer could start\n");
    REQUIRE(graphics.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
    REQUIRE(graphics.record.frames.empty());
}

TEST_CASE("Renderer display names", "[renderer]") {
    REQUIRE(RendererName::display(Backend::Vulkan, "Vulkan") == "Vulkan");
    REQUIRE(RendererName::display(Backend::OpenGL, "OpenGL 2.1") == "OpenGL");
    REQUIRE(RendererName::display(Backend::Metal, "Metal") == "Metal");
    REQUIRE(RendererName::display(Backend::Direct3D11, "Direct3D 11") == "Direct3D 11");
    REQUIRE(RendererName::display(Backend::Direct3D12, "Direct3D 12") == "Direct3D 12");
}

TEST_CASE("Any other renderer shows the rendering library's own name", "[renderer]") {
    REQUIRE(RendererName::display(Backend::Other, "Noop") == "Noop");
}

TEST_CASE("A hidden window is not drawn; the game sleeps until it is shown again", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{}, {Window::Event::Hidden}, {}, {}, {Window::Event::Shown}, {}, {}};
    const GameRun run{graphics};
    // Drawn after the 1st batch; not after Hidden (2nd) nor the batches waited for while hidden
    // (3rd, 4th); again after Shown (5th), the 6th and the 7th. The 8th poll finds none left and
    // closes.
    REQUIRE(graphics.record.frames.size() == 4);
    // The batches after Hidden (3rd, 4th, 5th) are waited for, not polled.
    REQUIRE(graphics.record.waits == 3);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("A hidden window can still be closed", "[game]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{Window::Event::Hidden}, {Window::Event::CloseRequested}};
    const GameRun run{graphics};
    REQUIRE(graphics.record.frames.empty());
    REQUIRE(graphics.record.waits == 1);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("The game starts the renderer it is given, after the start line", "[game][renderer]") {
    for (const auto backend : {Backend::Vulkan, Backend::OpenGL}) {
        FakeGraphicsSystem graphics;
        const GameRun run{graphics, backend};
        REQUIRE(graphics.record.outputWhenWindowOpened == versionLine());
        REQUIRE(graphics.record.requestedBackend == backend);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}

TEST_CASE("Given no renderer, the game lets the engine choose", "[game][renderer]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics, std::nullopt};
    REQUIRE(graphics.record.steps.size() >= 2);
    REQUIRE(graphics.record.steps.at(1) == "start renderer");
    REQUIRE_FALSE(graphics.record.requestedBackend.has_value());
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("A given renderer that fails to start: reported, nothing printed, window closed",
          "[game][renderer][startup]") {
    FakeGraphicsSystem graphics;
    graphics.rendererFailure = "the device could not be created";
    const GameRun run{graphics, Backend::Vulkan};
    REQUIRE(run.outcome() == Game::Outcome::ChosenRendererFailed);
    REQUIRE(run.out() == versionLine()); // the start line comes before the failure
    REQUIRE(run.err().empty());
    REQUIRE(graphics.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
}

TEST_CASE("Given no window mode, the game opens a window, not fullscreen", "[game][fullscreen]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE(graphics.record.windowSettings.mode == WindowMode::Windowed);
}

TEST_CASE("Asked for fullscreen, the game opens its window fullscreen, 1280x720 for later",
          "[game][fullscreen]") {
    for (const auto backend : {std::optional<Backend>{}, std::optional<Backend>{Backend::OpenGL}}) {
        FakeGraphicsSystem graphics;
        const GameRun run{graphics, backend, WindowMode::Fullscreen};
        const auto& settings = graphics.record.windowSettings;
        REQUIRE(settings.mode == WindowMode::Fullscreen);
        // The size the window gets when it first leaves fullscreen.
        REQUIRE(settings.size == Size{.width = 1280, .height = 720});
        REQUIRE(graphics.record.requestedBackend == backend);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}

TEST_CASE("Each F11 press switches fullscreen once, and the game keeps drawing",
          "[game][fullscreen]") {
    FakeGraphicsSystem graphics;
    graphics.events = {
        {Window::Event::FullscreenKeyPressed}, {}, {Window::Event::FullscreenKeyPressed}, {}};
    const GameRun run{graphics};
    REQUIRE(graphics.record.fullscreenSwitches == 2);
    REQUIRE(graphics.record.frames.size() == 4);
    REQUIRE(run.outcome() == Game::Outcome::Ended);
}

TEST_CASE("Switching fullscreen keeps the title with the renderer's name", "[game][fullscreen]") {
    FakeGraphicsSystem graphics;
    graphics.rendererName = "OpenGL";
    graphics.events = {{Window::Event::FullscreenKeyPressed}, {}};
    const GameRun run{graphics, Backend::OpenGL, WindowMode::Fullscreen};
    REQUIRE(graphics.record.fullscreenSwitches == 1);
    REQUIRE(
        graphics.record.titles ==
        std::vector<std::string>{"Wolfen " + std::string{Version::current().text()} + " (OpenGL)"});
}

TEST_CASE("In fullscreen, Esc and the close button still end the game", "[game][fullscreen]") {
    for (const auto quit : {Window::Event::EscapePressed, Window::Event::CloseRequested}) {
        FakeGraphicsSystem graphics;
        graphics.events = {{}, {quit}, {}};
        const GameRun run{graphics, std::nullopt, WindowMode::Fullscreen};
        REQUIRE(graphics.record.windowSettings.mode == WindowMode::Fullscreen);
        REQUIRE(graphics.record.frames.size() == 1);
        REQUIRE(run.outcome() == Game::Outcome::Ended);
    }
}
