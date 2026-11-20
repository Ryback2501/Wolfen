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

// What one run printed on its two output streams, and the exit code it ended with.
class RunOutput {
  public:
    [[nodiscard]] int exitCode() const {
        return exitCode_;
    }
    [[nodiscard]] std::string out() const {
        return out_.str();
    }
    [[nodiscard]] std::string err() const {
        return err_.str();
    }

  protected:
    // Runs `run` once, giving it the two streams to print to; `run` returns the exit code.
    template <typename Run> explicit RunOutput(const Run& run) : exitCode_{run(out_, err_)} {}

  private:
    std::ostringstream out_;
    std::ostringstream err_;
    int exitCode_; // declared after the streams, so they exist when the run starts
};

// Runs the game once with the given fake graphics system, and records what the game had printed
// when it opened the window.
class GameRun final : public RunOutput {
  public:
    explicit GameRun(FakeGraphicsSystem& graphics)
        : RunOutput{[&graphics](std::ostringstream& out, std::ostringstream& err) {
              graphics.watchedOutput = &out;
              game::Game game{out, err, graphics};
              return game.run();
          }} {}
    // Runs it with the given renderer choice, already checked by the launcher.
    GameRun(FakeGraphicsSystem& graphics, std::optional<engine::render::Backend> backend)
        : RunOutput{[&graphics, backend](std::ostringstream& out, std::ostringstream& err) {
              graphics.watchedOutput = &out;
              game::Game game{out, err, graphics};
              return game.run(backend);
          }} {}
};

// Runs the launcher once with the given start options and fake graphics system.
class LauncherRun final : public RunOutput {
  public:
    LauncherRun(FakeGraphicsSystem& graphics, std::initializer_list<std::string_view> args)
        : RunOutput{[&graphics, args](std::ostringstream& out, std::ostringstream& err) {
              const std::vector<std::string_view> words{args};
              game::Launcher launcher{out, err, graphics};
              return launcher.run(words);
          }} {}
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_RUNS_HPP
