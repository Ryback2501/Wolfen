#ifndef WOLFEN_GAME_GAME_HPP
#define WOLFEN_GAME_GAME_HPP

#include "app/graphics_system.hpp"

#include <iosfwd>
#include <span>
#include <string_view>

namespace wolfen::game {

// The Wolfen game. It talks to people through the two streams it is given (normal output and
// errors) and draws through the graphics system.
class Game {
  public:
    Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics);

    // Runs the game with the given start options (without the program's name) until it ends;
    // returns the program's exit code.
    int run(std::span<const std::string_view> args);
    // Runs the game with no start options.
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
