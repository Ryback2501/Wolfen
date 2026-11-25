#ifndef WOLFEN_TESTS_RUNS_HPP
#define WOLFEN_TESTS_RUNS_HPP

#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "game.hpp"
#include "launcher.hpp"
#include "platform/window_mode.hpp"
#include "render/backend.hpp"
#include "temp_folder.hpp"

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace wolfen::tests {

// The line the game prints when it starts, and for --version: "Wolfen vX.Y.Z".
inline std::string versionLine() {
    return "Wolfen " + std::string{engine::Version::current().text()} + "\n";
}

// What one run printed on its two output streams.
class RunOutput {
  public:
    [[nodiscard]] std::string out() const {
        return out_.str();
    }
    [[nodiscard]] std::string err() const {
        return err_.str();
    }

  protected:
    RunOutput() = default;

    // The streams the run prints to.
    [[nodiscard]] std::ostringstream& outStream() {
        return out_;
    }
    [[nodiscard]] std::ostringstream& errStream() {
        return err_;
    }

  private:
    std::ostringstream out_;
    std::ostringstream err_;
};

// Runs the game once with the given fake graphics system, and records what the game had printed
// when it opened the window.
class GameRun final : public RunOutput {
  public:
    explicit GameRun(FakeGraphicsSystem& graphics) : outcome_{watchedGame(graphics).run()} {}
    // Runs it with the given renderer choice, already checked by the launcher, in the given
    // window mode.
    GameRun(FakeGraphicsSystem& graphics, std::optional<engine::render::Backend> backend,
            engine::platform::WindowMode windowMode = engine::platform::WindowMode::Windowed)
        : outcome_{watchedGame(graphics).run(backend, windowMode, {})} {}

    [[nodiscard]] game::Game::Outcome outcome() const {
        return outcome_;
    }

  private:
    // A game printing to this run's streams; the graphics system records what it had printed
    // when the window opened.
    game::Game watchedGame(FakeGraphicsSystem& graphics) {
        graphics.watchedOutput = &outStream();
        return game::Game{outStream(), errStream(), graphics};
    }

    game::Game::Outcome outcome_;
};

// Runs the launcher once with the given start options and fake graphics system. Unless a game
// folder is given, the game's folder is a new temporary one, deleted after the run.
class LauncherRun final : public RunOutput {
  public:
    // Says that the words start with the program's name, as the operating system gives them.
    struct ProgramNameFirst {};

    // args: the words after the program's name.
    LauncherRun(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args)
        : exitCode_{launcher(graphics, ownFolder_.emplace().path())
                        .run(std::vector<std::string_view>{args})} {}

    // gameFolder: where the settings file is kept; nullopt when the system cannot say.
    LauncherRun(FakeGraphicsSystem& graphics,
                const std::optional<std::filesystem::path>& gameFolder,
                std::initializer_list<std::string_view> args)
        : exitCode_{launcher(graphics, gameFolder).run(std::vector<std::string_view>{args})} {}

    // words: the program's name and the words after it, for example {"wolfen", "--version"}.
    LauncherRun(FakeGraphicsSystem& graphics, ProgramNameFirst /*tag*/,
                std::initializer_list<std::string_view> words) {
        // main() receives non-const char* words, so the words are copied into strings this
        // constructor owns. The list points to them: argc pointers, then a null one.
        std::vector<std::string> copies(words.begin(), words.end());
        std::vector<char*> argv;
        argv.reserve(copies.size() + 1);
        for (std::string& copy : copies) {
            argv.push_back(copy.data());
        }
        argv.push_back(nullptr);
        const int argc = static_cast<int>(copies.size());
        exitCode_ = launcher(graphics, ownFolder_.emplace().path()).run(argc, argv.data());
    }

    [[nodiscard]] int exitCode() const {
        return exitCode_;
    }

  private:
    // A launcher printing to this run's streams.
    game::Launcher launcher(FakeGraphicsSystem& graphics,
                            const std::optional<std::filesystem::path>& gameFolder) {
        return game::Launcher{outStream(), errStream(), graphics, gameFolder};
    }

    // The game's folder, made only for a run that is not given one. Declared first, so it can be
    // made before the run.
    std::optional<TempFolder> ownFolder_;
    int exitCode_{};
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_RUNS_HPP
