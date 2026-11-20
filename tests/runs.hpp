#ifndef WOLFEN_TESTS_RUNS_HPP
#define WOLFEN_TESTS_RUNS_HPP

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
    LauncherRun(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args)
        : exitCode_{game::Launcher{outStream(), errStream(), graphics}.run(
              std::vector<std::string_view>{args})} {}

    [[nodiscard]] int exitCode() const {
        return exitCode_;
    }

  private:
    int exitCode_;
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_RUNS_HPP
