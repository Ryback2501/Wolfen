#ifndef WOLFEN_GAME_GAME_HPP
#define WOLFEN_GAME_GAME_HPP

#include "app/graphics_system.hpp"

#include <iosfwd>
#include <optional>
#include <string>

namespace wolfen::game {

// The Wolfen game. It talks to people through the two streams it is given (normal output and
// errors) and draws through the graphics system. The start options are read before it starts, by
// the launcher.
class Game {
  public:
    Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics);

    // "Wolfen" and the version, for example "Wolfen v1.2.3".
    [[nodiscard]] static std::string name();

    // Runs the game with the given renderer, or the best one for this computer when none is given,
    // until it ends; returns the program's exit code.
    int run(std::optional<engine::render::Backend> renderer);
    // Runs the game with the best renderer for this computer.
    int run();

  private:
    // The main loop: handles the window's events and draws a frame, until the game should end.
    static void play(engine::platform::Window& window, engine::render::Renderer& renderer);

    std::ostream* out_;
    std::ostream* err_;
    engine::app::GraphicsSystem* graphics_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_GAME_HPP
