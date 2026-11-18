#include "game/game.hpp"

#include <ostream>

namespace wolfen::game {

Game::Game(std::ostream& out) : out_{&out} {}

int Game::run() {
    out_->flush();
    return 0;
}

} // namespace wolfen::game
