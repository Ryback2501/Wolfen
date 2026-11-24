#include "fake_graphics.hpp"
#include "launcher.hpp"
#include "platform/window.hpp"
#include "render/backend.hpp"
#include "runs.hpp"
#include "temp_folder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using wolfen::engine::platform::WindowEvent;
using wolfen::engine::platform::WindowMode;
using wolfen::engine::render::Backend;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::LauncherRun;
using wolfen::tests::TempFolder;
using wolfen::tests::versionLine;

namespace {

// The usage text approved for #3, as changed by #4 (the --fullscreen line, and no keys), with the
// given list of available renderers.
std::string usage(std::string_view available) {
    return "Usage: wolfen [options]\n"
           "\n"
           "Options:\n"
           "  --renderer <name>  Draw with this renderer. Available on this computer: " +
           std::string{available} +
           "\n"
           "  --fullscreen       Start in borderless fullscreen.\n"
           "  --help             Show this help and exit.\n"
           "  --version          Show the version and exit.\n";
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

// A start in fullscreen with the given renderer choice: the start line, no error, exit 0.
void requireFullscreenStart(std::initializer_list<std::string_view> args,
                            std::optional<Backend> backend) {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, args};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphics.record.windowSettings.mode == WindowMode::Fullscreen);
    REQUIRE(graphics.record.requestedBackend == backend);
    REQUIRE(run.exitCode() == 0);
}

// A start with the given settings file (nullopt: none) that has no usable screen: the window is
// asked for no screen (the primary one), and a normal quit saves the screen it was on.
void requirePrimaryScreenThenSaved(const std::optional<std::string>& settingsText) {
    const TempFolder folder;
    if (settingsText) {
        folder.write("settings.ini", *settingsText);
    }
    FakeGraphicsSystem graphics;
    graphics.screen.name = "DP-1";
    const LauncherRun run{graphics, folder.path(), {"--fullscreen"}};
    REQUIRE(graphics.record.windowSettings.screen.name.empty());
    REQUIRE(folder.text("settings.ini") == "screen=DP-1\n");
    REQUIRE(run.err().empty());
    REQUIRE(run.exitCode() == 0);
}

// A start with the given options in a window that ends with the given quit event: the screen the
// window was on is saved in the game's folder, with no warning, exit 0.
void requireScreenSavedOnQuit(WindowEvent quit, std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    FakeGraphicsSystem graphics;
    graphics.screen.name = "HDMI-2";
    graphics.events = {{}, {quit}};
    const LauncherRun run{graphics, folder.path(), args};
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\n");
    REQUIRE(run.err().empty());
    REQUIRE(run.exitCode() == 0);
}

// A start with the given options and a saved screen: the window is asked for that screen.
void requireOpensOnSavedScreen(std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, folder.path(), args};
    REQUIRE(graphics.record.windowSettings.screen.name == "HDMI-2");
    REQUIRE(run.exitCode() == 0);
}

// Options that open no window, with a saved screen and with none: no window, the settings file is
// not changed, and none is created.
void requireSettingsLeftAlone(std::initializer_list<std::string_view> args) {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem graphics;
    graphics.screen.name = "DP-1";
    const LauncherRun run{graphics, folder.path(), args};
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\n");

    const TempFolder noFile;
    FakeGraphicsSystem again;
    const LauncherRun runAgain{again, noFile.path(), args};
    REQUIRE_FALSE(noFile.has("settings.ini"));
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
        REQUIRE(graphics.record.requestedBackend == backend);
        REQUIRE(graphics.record.steps.front() == "open window");
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("With no options the game lets the engine choose the renderer", "[options][renderer]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, {}};
    REQUIRE_FALSE(graphics.record.requestedBackend.has_value());
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
    const LauncherRun run{graphics, {"--renderer", "vulkan"}};
    REQUIRE(run.out() == versionLine()); // the start line comes before the failure
    REQUIRE(run.err() == "Renderer 'vulkan' could not be started on this computer.\n");
    REQUIRE(graphics.record.steps ==
            std::vector<std::string>{"open window", "start renderer", "close window"});
    REQUIRE(run.exitCode() == 1);
}

TEST_CASE("A start failure exits with 1", "[options][startup]") {
    FakeGraphicsSystem graphics;
    graphics.windowFailure = "no display available";
    const LauncherRun run{graphics, {}};
    REQUIRE(run.err() == "Wolfen could not start: no display available\n");
    REQUIRE(run.exitCode() == 1);
}

TEST_CASE("Words as the system gives them: 'wolfen --version' prints the version line, exit 0",
          "[options][version][main]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, LauncherRun::ProgramNameFirst{}, {"wolfen", "--version"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err().empty());
    REQUIRE(graphics.record.steps.empty());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("Words as the system gives them: 'wolfen' alone plays with no renderer choice, exit 0",
          "[options][renderer][main]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, LauncherRun::ProgramNameFirst{}, {"wolfen"}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(graphics.record.steps.front() == "open window");
    REQUIRE_FALSE(graphics.record.requestedBackend.has_value());
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The usage lists --fullscreen right after --renderer, and mentions no keys",
          "[options][help][fullscreen]") {
    FakeGraphicsSystem graphics;
    const std::string out = LauncherRun{graphics, {"--help"}}.out();
    REQUIRE(out == usageHere);
    REQUIRE(out.find("F11") == std::string::npos);
    REQUIRE(out.find("Esc") == std::string::npos);
}

TEST_CASE("The --fullscreen option starts the game fullscreen, in any order with --renderer",
          "[options][fullscreen]") {
    requireFullscreenStart({"--fullscreen"}, std::nullopt);
    requireFullscreenStart({"--fullscreen", "--renderer", "opengl"}, Backend::OpenGL);
    requireFullscreenStart({"--renderer", "opengl", "--fullscreen"}, Backend::OpenGL);
}

TEST_CASE("Without --fullscreen the game starts in a window", "[options][fullscreen]") {
    for (const auto args : {std::initializer_list<std::string_view>{},
                            std::initializer_list<std::string_view>{"--renderer", "vulkan"}}) {
        FakeGraphicsSystem graphics;
        const LauncherRun run{graphics, args};
        REQUIRE(graphics.record.windowSettings.mode == WindowMode::Windowed);
        REQUIRE(run.exitCode() == 0);
    }
}

TEST_CASE("In fullscreen, Esc quits with exit code 0", "[options][fullscreen]") {
    FakeGraphicsSystem graphics;
    graphics.events = {{}, {WindowEvent::EscapePressed}};
    const LauncherRun run{graphics, {"--fullscreen"}};
    REQUIRE(graphics.record.windowSettings.mode == WindowMode::Fullscreen);
    REQUIRE(graphics.record.frames.size() == 1);
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("The --fullscreen option given twice", "[options][fullscreen]") {
    FakeGraphicsSystem graphics;
    requireOptionError(graphics, {"--fullscreen", "--fullscreen"},
                       "Option '--fullscreen' given twice");
    // Only the first error, reading left to right, is printed.
    FakeGraphicsSystem thenUnknown;
    requireOptionError(thenUnknown, {"--fullscreen", "--fullscreen", "--bogus"},
                       "Option '--fullscreen' given twice");
}

TEST_CASE("A normal quit saves the screen the window was on in the game's folder, in a window or "
          "fullscreen",
          "[settings]") {
    for (const auto quit : {WindowEvent::CloseRequested, WindowEvent::EscapePressed}) {
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
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, folder.path(), {"--fullscreen"}};
    REQUIRE(graphics.record.windowSettings.screen.name == "HDMI-2");
}

TEST_CASE("Fullscreen is not remembered: after quitting fullscreen, the next start opens a window "
          "on the same screen",
          "[settings][fullscreen]") {
    const TempFolder folder;
    FakeGraphicsSystem first;
    first.screen.name = "HDMI-2";
    first.events = {{WindowEvent::FullscreenKeyPressed}, {WindowEvent::CloseRequested}};
    const LauncherRun firstRun{first, folder.path(), {}};
    REQUIRE(first.record.fullscreenSwitches == 1);

    FakeGraphicsSystem next;
    const LauncherRun nextRun{next, folder.path(), {}};
    REQUIRE(next.record.windowSettings.mode == WindowMode::Windowed);
    REQUIRE(next.record.windowSettings.screen.name == "HDMI-2");
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
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, folder.path(), {}};
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err() == "Warning: could not save settings to " +
                             (folder.path() / "settings.ini").string() + "\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("When the game's folder is unknown: the primary screen, one warning line on quit, exit 0",
          "[settings]") {
    FakeGraphicsSystem graphics;
    const LauncherRun run{graphics, std::nullopt, {"--fullscreen"}};
    REQUIRE(graphics.record.windowSettings.screen.name.empty());
    REQUIRE(run.out() == versionLine());
    REQUIRE(run.err() == "Warning: could not save settings: the game's folder is unknown\n");
    REQUIRE(run.exitCode() == 0);
}

TEST_CASE("A screen the system gives no name is saved as screen= with no name", "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=HDMI-2\n");
    FakeGraphicsSystem graphics;
    graphics.screen.name = "";
    const LauncherRun run{graphics, folder.path(), {}};
    REQUIRE(folder.text("settings.ini") == "screen=\n");
    REQUIRE(run.exitCode() == 0);
}
