#ifndef WOLFEN_GAME_GAME_HPP
#define WOLFEN_GAME_GAME_HPP

#include "app/graphics_system.hpp"

#include <iosfwd>

namespace wolfen::game {

// The Wolfen game. It talks to people through the two streams it is given (normal output and
// errors) and draws through the graphics system.
class Game {
  public:
    Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics);

    // Runs the game until it ends; returns the program's exit code.
    int run();

  private:
    std::ostream* out_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_GAME_HPP
