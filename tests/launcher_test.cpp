#include "core/point.hpp"
#include "fake_graphics.hpp"
#include "launcher.hpp"
#include "platform/screen.hpp"
#include "platform/window.hpp"
#include "platform/window_mode.hpp"
#include "render/graphics_api.hpp"
#include "runs.hpp"
#include "temp_folder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using wolfen::engine::Point;
using wolfen::engine::platform::Screen;
using wolfen::engine::platform::Window;
using wolfen::engine::platform::WindowMode;
using wolfen::engine::render::GraphicsApi;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::LauncherRun;
using wolfen::tests::TempFolder;
using wolfen::tests::versionLine;

namespace {

// The usage text approved for #3, as changed by #4 (the --fullscreen line, and no keys) and #8 (the
// --software line), with the given list of available graphics APIs.
std::string usage(std::string_view available) {
    return "Usage: wolfen [options]\n"
           "\n"
           "Options:\n"
           "  --renderer <name>  Draw with this renderer. Available on this computer: " +
           std::string{available} +
           "\n"
           "  --fullscreen       Start in borderless fullscreen.\n"
           "  --software         Draw with the CPU instead of the graphics card.\n"
           "  --help             Show this help and exit.\n"
           "  --version          Show the version and exit.\n";
}

const std::string usageHere = usage("vulkan, opengl"); // the fake's default available graphics APIs

// The one line shown after an option error, instead of the usage.
const std::string hintLine = "Run 'wolfen --help' to see the available options.\n";

// An option error: the message on stderr, then the hint line; nothing on stdout, no window, exit 1.
void requireOptionError(FakeGraphicsSystem& graphicsSystem,
                        std::initializer_list<std::string_view> args, const std::string& message) {
    const LauncherRun run{graphicsSystem, args};
    REQUIRE(run.err() == message + "\n" + hintLine);
    REQUIRE(run.out().empty());
    REQUIRE(graphicsSystem.record.steps.empty());
    REQUIRE(run.exitCode() == 1);
}

// A start in fullscreen with the given graphics API choice: the start line, no error, exit 0.
void requireFullscreenStart(std::initializer_list<std::string_view> args,
                            std::optional<GraphicsApi> graphicsApi) {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, args};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.windowSettings.mode == WindowMode::Fullscreen);
    REQUIRE(graphicsSystem.record.requestedGraphicsApi == graphicsApi);
    REQUIRE(run.exitCode() == 0);
}

// A start with the given settings file (nullopt: none) that has no usable screen: the window is
// asked for no screen (the primary one), and a normal quit saves the screen it was on.
void requirePrimaryScreenThenSaved(const std::optional<std::string>& settingsText) {
    const TempFolder folder;
    if (settingsText) {
        folder.write("settings.ini", *settingsText);
    }
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.screen = {.name = "DP-1", .position = Point{.x = 0, .y = 0}};
    const LauncherRun run{graphicsSystem, folder.path(), {"--fullscreen"}};
    REQUIRE(graphicsSystem.record.windowSettings.screen.name.empty());
    REQUIRE(folder.text("settings.ini") == "screen=DP-1\nscreen_position=0,0\n");
    REQUIRE(run.err().empty());
    REQUIRE(run.exitCode() == 0);
}

// A start with the given options in a window that ends with the given quit event: the screen the
// window was on is saved in the game's folder, with no warning, exit 0.
void requireScreenSavedOnQuit(Window::Event quit, std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.screen = {.name = "HDMI-2", .position = Point{.x = 1920, .y = 0}};
    graphicsSystem.events = {{}, {quit}};
    const LauncherRun run{graphicsSystem, folder.path(), args};
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\nscreen_position=1920,0\n");
    REQUIRE(run.err().empty());
    REQUIRE(run.exitCode() == 0);
}

// A start with the given options and a saved screen: the window is asked for that screen, by its
// name and its position.
void requireOpensOnSavedScreen(std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\nscreen_position=1920,0\n");
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, folder.path(), args};
    REQUIRE(graphicsSystem.record.windowSettings.screen ==
            Screen{.name = "HDMI-2", .position = Point{.x = 1920, .y = 0}});
    REQUIRE(run.exitCode() == 0);
}

// Options that open no window, with a saved screen and with none: no window, the settings file is
// not changed, and none is created.
void requireSettingsLeftAlone(std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.screen.name = "DP-1";
    const LauncherRun run{graphicsSystem, folder.path(), args};
    REQUIRE(graphicsSystem.record.steps.empty());
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\n");

    const TempFolder noFile;
    FakeGraphicsSystem again;
    const LauncherRun runAgain{again, noFile.path(), args};
    REQUIRE_FALSE(noFile.has("settings.ini"));
}

// The window title with the given renderer text: "Wolfen vX.Y.Z (<renderer>)".
std::string titleWith(std::string_view renderer) {
    std::string title = versionLine();
    title.pop_back(); // the line's newline
    return title + " (" + std::string{renderer} + ")";
}

// The number of times the steps hold the given one.
std::ptrdiff_t countOf(const std::vector<std::string>& steps, std::string_view step) {
    return std::ranges::count(steps, step);
}

// A start with the given options on a computer where only Vulkan can draw with the CPU: the game
// draws with the CPU on Vulkan, never starts a renderer on the graphics card, says so in the title,
// and exits with 0.
void requireSoftwareStart(std::initializer_list<std::string_view> args) {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.cpuGraphicsApis = {GraphicsApi::Vulkan};
    const LauncherRun run{graphicsSystem, args};
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.cpuTries == std::vector<GraphicsApi>{GraphicsApi::Vulkan});
    REQUIRE(countOf(graphicsSystem.record.steps, "start renderer") == 0);
    REQUIRE(graphicsSystem.record.titles ==
            std::vector<std::string>{titleWith("Vulkan, software")});
    REQUIRE(run.exitCode() == 0);
}

// A start with --software where no renderer can draw with the CPU: one message, no window left
// open, exit 1.
void requireNoCpuRenderer(FakeGraphicsSystem& graphicsSystem) {
    const LauncherRun run{graphicsSystem, {"--software"}};
    REQUIRE(run.err() == "No renderer can draw with the CPU on this computer.\n");
    REQUIRE(countOf(graphicsSystem.record.steps, "open window") ==
            countOf(graphicsSystem.record.steps, "close window"));
    REQUIRE(countOf(graphicsSystem.record.steps, "start renderer") == 0);
    REQUIRE(run.exitCode() == 1);
}

} // namespace

TEST_CASE("The --help option prints the usage, opens no window and exits with 0",
          "[options][help]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, {"--help"}};
    REQUIRE(run.out() == usageHere); // the usage only: no start line
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --help option wins over a wrong option and over --version", "[options][help]") {
    for (const auto args : {std::initializer_list<std::string_view>{"--bogus", "--help"},
                            std::initializer_list<std::string_view>{"--version", "--help"},
                            std::initializer_list<std::string_view>{"--renderer", "--help"}}) {
        FakeGraphicsSystem graphicsSystem;
        const LauncherRun run{graphicsSystem, args};
        REQUIRE(run.out() == usageHere);
        REQUIRE(run.err().empty());
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("The usage lists only the renderers available on this computer, in a fixed order",
          "[options][help]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.available = {GraphicsApi::OpenGL, GraphicsApi::Vulkan, GraphicsApi::Direct3D12};
    REQUIRE(LauncherRun{graphicsSystem, {"--help"}}.out() == usage("vulkan, opengl, d3d12"));

    FakeGraphicsSystem mac;
    mac.available = {GraphicsApi::Metal};
    REQUIRE(LauncherRun{mac, {"--help"}}.out() == usage("metal"));

    FakeGraphicsSystem none;
    none.available = {};
    REQUIRE(LauncherRun{none, {"--help"}}.out() == usage("(none)"));
}

TEST_CASE("The --version option prints exactly the version line, opens no window and exits with 0",
          "[options][version]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, {"--version"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --version option wins over wrong and repeated options", "[options][version]") {
    for (const auto args :
         {std::initializer_list<std::string_view>{"--version", "--bogus"},
          std::initializer_list<std::string_view>{"--renderer", "foo", "--version"},
          std::initializer_list<std::string_view>{"--version", "--version"},
          std::initializer_list<std::string_view>{"--renderer", "--version"}}) {
        FakeGraphicsSystem graphicsSystem;
        const LauncherRun run{graphicsSystem, args};
        REQUIRE(run.out() == versionLine());
        REQUIRE(run.err().empty());
        REQUIRE(graphicsSystem.record.steps.empty());
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("The --renderer option starts only the chosen renderer, after the start line",
          "[options][renderer]") {
    for (const auto& [name, graphicsApi] :
         {std::pair{"vulkan", GraphicsApi::Vulkan}, std::pair{"opengl", GraphicsApi::OpenGL}}) {
        FakeGraphicsSystem graphicsSystem;
        const LauncherRun run{graphicsSystem, {"--renderer", name}};
        REQUIRE(run.out() == versionLine());
        REQUIRE(graphicsSystem.record.requestedGraphicsApi == graphicsApi);
        REQUIRE(graphicsSystem.record.steps.front() == "open window");
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("With no options the game lets the engine choose the renderer", "[options][renderer]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, {}};
    REQUIRE_FALSE(graphicsSystem.record.requestedGraphicsApi.has_value());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("A renderer that is not available on this computer", "[options][renderer]") {
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--renderer", "metal"},
                       "Renderer 'metal' is not available on this computer. Available: vulkan, "
                       "opengl");
}

TEST_CASE("A renderer name Wolfen does not know", "[options][renderer]") {
    for (const auto name : {std::string_view{"foo"}, std::string_view{"Vulkan"}}) {
        FakeGraphicsSystem graphicsSystem;
        requireOptionError(graphicsSystem, {"--renderer", name},
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
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--renderer", "opengl", "--renderer", "vulkan"},
                       "Option '--renderer' given twice");
}

TEST_CASE("Unknown options, including the --option=value form", "[options]") {
    for (const auto word : {std::string_view{"--bogus"}, std::string_view{"foo"},
                            std::string_view{"--renderer=vulkan"}}) {
        FakeGraphicsSystem graphicsSystem;
        requireOptionError(graphicsSystem, {word}, "Unknown option '" + std::string{word} + "'");
    }
}

TEST_CASE("Only the first error, reading left to right, is printed", "[options]") {
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--bogus", "--renderer", "foo"},
                       "Unknown option '--bogus'");
}

TEST_CASE("A chosen renderer that fails to start: its own message, exit 1, window closed",
          "[options][renderer][startup]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererFailure = "the device could not be created";
    const LauncherRun run{graphicsSystem, {"--renderer", "vulkan"}};
    REQUIRE(run.out() == versionLine()); // the start line comes before the failure
    REQUIRE(run.err() == "Renderer 'vulkan' could not be started on this computer.\n");
    REQUIRE(graphicsSystem.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
    REQUIRE(run.exitCode() == 1);
}

TEST_CASE("A start failure exits with 1", "[options][startup]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.windowFailure = "no display available";
    const LauncherRun run{graphicsSystem, {}};
    REQUIRE(run.err() == "Wolfen could not start: no display available\n");
    REQUIRE(run.exitCode() == 1);
}

TEST_CASE("Words as the system gives them: 'wolfen --version' prints the version line, exit 0",
          "[options][version][main]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, LauncherRun::ProgramNameFirst{}, {"wolfen", "--version"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphicsSystem.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("Words as the system gives them: 'wolfen' alone plays with no renderer choice, exit 0",
          "[options][renderer][main]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, LauncherRun::ProgramNameFirst{}, {"wolfen"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(graphicsSystem.record.steps.front() == "open window");
    REQUIRE_FALSE(graphicsSystem.record.requestedGraphicsApi.has_value());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The usage lists --fullscreen right after --renderer, and mentions no keys",
          "[options][help][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    const std::string out = LauncherRun{graphicsSystem, {"--help"}}.out();
    REQUIRE(out == usageHere);
    REQUIRE(out.find("F11") == std::string::npos);
    REQUIRE(out.find("Esc") == std::string::npos);
}

TEST_CASE("The --fullscreen option starts the game fullscreen, in any order with --renderer",
          "[options][fullscreen]") {
    requireFullscreenStart({"--fullscreen"}, std::nullopt);
    requireFullscreenStart({"--fullscreen", "--renderer", "opengl"}, GraphicsApi::OpenGL);
    requireFullscreenStart({"--renderer", "opengl", "--fullscreen"}, GraphicsApi::OpenGL);
}

TEST_CASE("Without --fullscreen the game starts in a window", "[options][fullscreen]") {
    for (const auto args : {std::initializer_list<std::string_view>{},
                            std::initializer_list<std::string_view>{"--renderer", "vulkan"}}) {
        FakeGraphicsSystem graphicsSystem;
        const LauncherRun run{graphicsSystem, args};
        REQUIRE(graphicsSystem.record.windowSettings.mode == WindowMode::Windowed);
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("In fullscreen, Esc quits with exit code 0", "[options][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.events = {{}, {Window::Event::EscapePressed}};
    const LauncherRun run{graphicsSystem, {"--fullscreen"}};
    REQUIRE(graphicsSystem.record.windowSettings.mode == WindowMode::Fullscreen);
    REQUIRE(graphicsSystem.record.frames.size() == 1);
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --fullscreen option given twice", "[options][fullscreen]") {
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--fullscreen", "--fullscreen"},
                       "Option '--fullscreen' given twice");
    // Only the first error, reading left to right, is printed.
    FakeGraphicsSystem thenUnknown;
    requireOptionError(thenUnknown, {"--fullscreen", "--fullscreen", "--bogus"},
                       "Option '--fullscreen' given twice");
}

TEST_CASE("A normal quit saves the screen the window was on in the game's folder, in a window or "
          "fullscreen",
          "[settings]") {
    for (const auto quit : {Window::Event::CloseRequested, Window::Event::EscapePressed}) {
        requireScreenSavedOnQuit(quit, {});
        requireScreenSavedOnQuit(quit, {"--fullscreen"});
    }
}

TEST_CASE("The game opens on the saved screen, in a window or fullscreen", "[settings]") {
    requireOpensOnSavedScreen({});
    requireOpensOnSavedScreen({"--fullscreen"});
}

TEST_CASE("With no settings file, an empty or broken one, or no name, the game opens on the "
          "primary screen and saves a correct file",
          "[settings]") {
    requirePrimaryScreenThenSaved(std::nullopt);
    for (const std::string text : {"", "garbage", "screen="}) {
        requirePrimaryScreenThenSaved(text);
    }
}

TEST_CASE("A saved name with spaces around it, Windows line endings and another line is found",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=  HDMI-2  \r\nfoo=bar\r\n");
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, folder.path(), {"--fullscreen"}};
    REQUIRE(graphicsSystem.record.windowSettings.screen.name == "HDMI-2");
}

TEST_CASE("Fullscreen is not remembered: after quitting fullscreen, the next start opens a window "
          "on the same screen",
          "[settings][fullscreen]") {
    const TempFolder folder;
    FakeGraphicsSystem first;
    first.screen = {.name = "HDMI-2", .position = Point{.x = 1920, .y = 0}};
    first.events = {{Window::Event::FullscreenKeyPressed}, {Window::Event::CloseRequested}};
    const LauncherRun firstRun{first, folder.path(), {}};
    REQUIRE(first.record.fullscreenSwitches == 1);

    FakeGraphicsSystem next;
    const LauncherRun nextRun{next, folder.path(), {}};
    REQUIRE(next.record.windowSettings.mode == WindowMode::Windowed);
    REQUIRE(next.record.windowSettings.screen == first.screen);
}

TEST_CASE("With --help, --version or a wrong option, no window opens and the settings file is "
          "left alone",
          "[settings][options]") {
    requireSettingsLeftAlone({"--help"});
    requireSettingsLeftAlone({"--version"});
    requireSettingsLeftAlone({"--renderer", "nope"});
    requireSettingsLeftAlone({"--bogus"});
}

TEST_CASE("A start that fails never writes the settings file", "[settings][startup]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem rendererFails;
    rendererFails.screen.name = "DP-1";
    rendererFails.rendererFailure = "the device could not be created";
    const LauncherRun chosenRun{rendererFails, folder.path(), {"--renderer", "vulkan"}};
    REQUIRE(chosenRun.exitCode() == 1);
    const LauncherRun bestRun{rendererFails, folder.path(), {}};
    REQUIRE(bestRun.exitCode() == 1);
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\n");

    const TempFolder noFile;
    FakeGraphicsSystem windowFails;
    windowFails.windowFailure = "no display available";
    const LauncherRun failedRun{windowFails, noFile.path(), {}};
    REQUIRE(failedRun.exitCode() == 1);
    REQUIRE_FALSE(noFile.has("settings.ini"));
}

TEST_CASE("When the settings file cannot be written: one warning line with its full path, exit 0",
          "[settings]") {
    const TempFolder folder;
    std::filesystem::create_directory(folder.path() / "settings.ini");
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, folder.path(), {}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err() == "Warning: could not save settings to " +
                             (folder.path() / "settings.ini").string() + "\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("When the game's folder is unknown: the primary screen, one warning line on quit, exit 0",
          "[settings]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, std::nullopt, {"--fullscreen"}};
    REQUIRE(graphicsSystem.record.windowSettings.screen.name.empty());
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err() == "Warning: could not save settings: the game's folder is unknown\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("A screen the system gives no name is saved as screen= with no name", "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.screen = {.name = "", .position = Point{.x = 0, .y = 0}};
    const LauncherRun run{graphicsSystem, folder.path(), {}};
    REQUIRE(folder.text("settings.ini") == "screen=\nscreen_position=0,0\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("A screen whose position the system cannot give is saved with an empty screen_position=",
          "[settings]") {
    const TempFolder folder;
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.screen = {.name = "HDMI-2", .position = std::nullopt};
    const LauncherRun run{graphicsSystem, folder.path(), {}};
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\nscreen_position=\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The usage lists --software right after --fullscreen", "[options][help][software]") {
    FakeGraphicsSystem graphicsSystem;
    const LauncherRun run{graphicsSystem, {"--help"}};
    REQUIRE(run.out() == usageHere);
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("With --software the game draws with the CPU, and the title says so", "[software]") {
    requireSoftwareStart({"--software"});
}

TEST_CASE("With --software the renderers that can draw with the CPU are tried in the order Vulkan, "
          "Direct3D 11, Direct3D 12, each in a window of its own",
          "[software]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.available = {GraphicsApi::Direct3D12, GraphicsApi::OpenGL,
                                GraphicsApi::Direct3D11, GraphicsApi::Vulkan};
    graphicsSystem.cpuGraphicsApis = {GraphicsApi::Direct3D12};
    const LauncherRun run{graphicsSystem, {"--software"}};
    REQUIRE(graphicsSystem.record.cpuTries == std::vector<GraphicsApi>{GraphicsApi::Vulkan,
                                                                       GraphicsApi::Direct3D11,
                                                                       GraphicsApi::Direct3D12});
    // A failed try closes its window: a renderer on another graphics API never reuses it.
    REQUIRE(graphicsSystem.record.steps ==
            std::vector<std::string>{"open window", "start CPU renderer", "close window",
                                     "open window", "start CPU renderer", "close window",
                                     "open window", "start CPU renderer", "close renderer",
                                     "close window"});
    REQUIRE(graphicsSystem.record.titles ==
            std::vector<std::string>{titleWith("Direct3D 12, software")});
    REQUIRE(run.err().empty());
    REQUIRE(run.exitCode() == 0);

    // Only the graphics APIs available on this computer are tried.
    FakeGraphicsSystem windows;
    windows.available = {GraphicsApi::OpenGL, GraphicsApi::Direct3D11};
    windows.cpuGraphicsApis = {GraphicsApi::Direct3D11};
    const LauncherRun windowsRun{windows, {"--software"}};
    REQUIRE(windows.record.cpuTries == std::vector<GraphicsApi>{GraphicsApi::Direct3D11});
    REQUIRE(windows.record.titles == std::vector<std::string>{titleWith("Direct3D 11, software")});
}

TEST_CASE("With --software, --renderer is ignored completely", "[software][renderer]") {
    requireSoftwareStart({"--renderer", "opengl", "--software"});
    requireSoftwareStart({"--software", "--renderer", "opengl"});
    requireSoftwareStart({"--renderer", "foo", "--software"});
    requireSoftwareStart({"--renderer", "--software"});
    requireSoftwareStart({"--software", "--renderer"});
    requireSoftwareStart({"--renderer", "vulkan", "--renderer", "opengl", "--software"});
}

TEST_CASE("The --software option combines with --fullscreen in any order",
          "[software][fullscreen]") {
    requireSoftwareStart({"--software", "--fullscreen"});
    requireSoftwareStart({"--fullscreen", "--software"});
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.cpuGraphicsApis = {GraphicsApi::Vulkan};
    graphicsSystem.events = {{Window::Event::FullscreenKeyPressed}, {}};
    const LauncherRun run{graphicsSystem, {"--fullscreen", "--software"}};
    REQUIRE(graphicsSystem.record.windowSettings.mode == WindowMode::Fullscreen);
    REQUIRE(graphicsSystem.record.fullscreenSwitches == 1);
    REQUIRE(graphicsSystem.record.titles ==
            std::vector<std::string>{titleWith("Vulkan, software")});
}

TEST_CASE("The --software option given twice", "[software][options]") {
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--software", "--software"},
                       "Option '--software' given twice");
}

TEST_CASE(
    "With --software, an unknown option is still an error, and --help and --version still win",
    "[software][options]") {
    FakeGraphicsSystem graphicsSystem;
    requireOptionError(graphicsSystem, {"--software", "--bogus"}, "Unknown option '--bogus'");

    FakeGraphicsSystem helpGraphicsSystem;
    const LauncherRun help{helpGraphicsSystem, {"--help", "--software"}};
    REQUIRE(help.out() == usageHere);
    REQUIRE(help.exitCode() == 0);

    FakeGraphicsSystem versionGraphicsSystem;
    const LauncherRun version{versionGraphicsSystem, {"--version", "--software"}};
    REQUIRE(version.out() == versionLine());
    REQUIRE(version.exitCode() == 0);
}

TEST_CASE("When no renderer can draw with the CPU: one message, no window left open, exit 1",
          "[software]") {
    FakeGraphicsSystem noneAvailable; // as on macOS: no renderer that can draw with the CPU
    noneAvailable.available = {GraphicsApi::OpenGL, GraphicsApi::Metal};
    requireNoCpuRenderer(noneAvailable);
    REQUIRE(noneAvailable.record.cpuTries.empty());

    FakeGraphicsSystem noneStarts; // as on Linux without a CPU Vulkan driver
    noneStarts.available = {GraphicsApi::Vulkan, GraphicsApi::OpenGL};
    requireNoCpuRenderer(noneStarts);
    REQUIRE(noneStarts.record.cpuTries == std::vector<GraphicsApi>{GraphicsApi::Vulkan});
}

TEST_CASE("Without --software, a renderer on the graphics card has no ', software' in the title",
          "[software]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererDisplayName = "Vulkan";
    const LauncherRun run{graphicsSystem, {}};
    REQUIRE(graphicsSystem.record.cpuTries.empty());
    REQUIRE(graphicsSystem.record.titles == std::vector<std::string>{titleWith("Vulkan")});
}

TEST_CASE("Without --software, a renderer that ends up on a CPU device says so in the title",
          "[software]") {
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.rendererDisplayName = "Vulkan";
    graphicsSystem.rendererDrawsWithCpu = true; // as on a virtual machine with no graphics card
    const LauncherRun run{graphicsSystem, {}};
    REQUIRE(graphicsSystem.record.titles ==
            std::vector<std::string>{titleWith("Vulkan, software")});
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("With --software the saved screen is used, and saved again after a normal quit",
          "[software][settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\nscreen_position=1920,0\n");
    FakeGraphicsSystem graphicsSystem;
    graphicsSystem.cpuGraphicsApis = {GraphicsApi::Vulkan};
    graphicsSystem.screen = {.name = "DP-1", .position = Point{.x = 0, .y = 0}};
    const LauncherRun run{graphicsSystem, folder.path(), {"--software"}};
    REQUIRE(graphicsSystem.record.windowSettings.screen ==
            Screen{.name = "HDMI-2", .position = Point{.x = 1920, .y = 0}});
    REQUIRE(folder.text("settings.ini") == "screen=DP-1\nscreen_position=0,0\n");
    REQUIRE(run.exitCode() == 0);
}
