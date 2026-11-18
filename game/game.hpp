#pragma once

#include <iosfwd>

namespace wolfen::game {

// The Wolfen game. It writes what it has to say to the stream it is given.
class Game {
  public:
    explicit Game(std::ostream& out);

    // Runs the game until it ends; returns the program's exit code.
    int run();

  private:
    std::ostream* out_;
};

} // namespace wolfen::game
