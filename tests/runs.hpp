#ifndef WOLFEN_TESTS_RUNS_HPP
#define WOLFEN_TESTS_RUNS_HPP

#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "game.hpp"
#include "launcher.hpp"
#include "render/renderer_name.hpp"

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
    // Runs it with the given renderer choice, already checked by the launcher.
    GameRun(FakeGraphicsSystem& graphics, std::optional<engine::render::Backend> backend)
        : outcome_{watchedGame(graphics).run(backend)} {}

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

// Runs the launcher once with the given start options and fake graphics system.
class LauncherRun final : public RunOutput {
  public:
    // Says that the words start with the program's name, as the operating system gives them.
    struct ProgramNameFirst {};

    // args: the words after the program's name.
    LauncherRun(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args)
        : exitCode_{game::Launcher{outStream(), errStream(), graphics}.run(
              std::vector<std::string_view>{args})} {}

    // words: the program's name and the words after it, for example {"wolfen", "--version"}.
    LauncherRun(FakeGraphicsSystem& graphics, ProgramNameFirst /*tag*/,
                std::initializer_list<std::string_view> words) {
        // Copies of the words that the launcher may change, and the list of them that main()
        // receives: argc pointers, then a null one.
        std::vector<std::string> copies(words.begin(), words.end());
        std::vector<char*> argv;
        for (std::string& copy : copies) {
            argv.push_back(copy.data());
        }
        argv.push_back(nullptr);
        const int argc = static_cast<int>(copies.size());
        exitCode_ = game::Launcher{outStream(), errStream(), graphics}.run(argc, argv.data());
    }

    [[nodiscard]] int exitCode() const {
        return exitCode_;
    }

  private:
    int exitCode_{};
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_RUNS_HPP
