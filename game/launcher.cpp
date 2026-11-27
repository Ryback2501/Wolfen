#include "launcher.hpp"

#include "command_line.hpp"
#include "game.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wolfen::game {

using engine::app::GraphicsSystem;
using engine::platform::Screen;
using engine::render::GraphicsApi;

Launcher::Launcher(std::ostream& out, std::ostream& err, GraphicsSystem& graphics,
                   std::optional<std::filesystem::path> gameFolder)
    : out_{&out}, err_{&err}, graphics_{&graphics}, gameFolder_{std::move(gameFolder)} {}

int Launcher::run(std::span<const std::string_view> args) {
    const std::vector<GraphicsApi> availableGraphicsApis = graphics_->availableGraphicsApis();
    const CommandLine commandLine{args, availableGraphicsApis};

    switch (commandLine.action()) {
    case CommandLine::Action::Help:
        *out_ << CommandLine::usage(availableGraphicsApis);
        return 0;
    case CommandLine::Action::Version:
        *out_ << Game::name() << '\n';
        return 0;
    case CommandLine::Action::Error:
        *err_ << commandLine.error() << '\n' << CommandLine::hint() << '\n';
        return 1;
    case CommandLine::Action::Run:
        break;
    }

    // Read only now, just before the window opens: the screen to open on.
    std::optional<SettingsFile> settingsFile;
    if (gameFolder_) {
        settingsFile.emplace(*gameFolder_ / "settings.ini");
    }
    const Screen screen = settingsFile ? settingsFile->screen() : Screen{};

    const std::optional<GraphicsApi> graphicsApi = commandLine.graphicsApi();
    Game game{*out_, *err_, *graphics_};
    const Game::Outcome outcome = commandLine.isSoftwareAsked()
                                      ? game.runOnCpu(commandLine.windowMode(), screen)
                                      : game.run(graphicsApi, commandLine.windowMode(), screen);
    switch (outcome) {
    case Game::Outcome::Ended:
        saveScreen(settingsFile, game.screenAtQuit());
        return 0;
    case Game::Outcome::ChosenRendererFailed:
        *err_ << "Renderer '"
              << CommandLine::graphicsApiName(graphicsApi.value_or(GraphicsApi::Other))
              << "' could not be started on this computer.\n";
        return 1;
    case Game::Outcome::NoCpuRenderer:
        *err_ << "No renderer can draw with the CPU on this computer.\n";
        return 1;
    case Game::Outcome::CouldNotStart:
        break; // the game has already said why
    }
    return 1;
}

void Launcher::saveScreen(const std::optional<SettingsFile>& settingsFile,
                          const Screen& screen) const {
    if (!settingsFile) {
        *err_ << "Warning: could not save settings: the game's folder is unknown\n";
        return;
    }
    if (!settingsFile->save(screen)) {
        // The path's own characters (UTF-8), whatever the system's code page.
        const std::u8string path = settingsFile->path().u8string();
        *err_ << "Warning: could not save settings to " << std::string(path.begin(), path.end())
              << '\n';
    }
}

int Launcher::run(int argc, char** argv) {
    // The start options, without the program's own name.
    const std::span<char*> words{argv, static_cast<std::size_t>(argc)};
    const std::vector<std::string_view> args(words.empty() ? words.end() : words.begin() + 1,
                                             words.end());
    return run(args);
}

} // namespace wolfen::game
