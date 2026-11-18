#include "game.hpp"

#include "core/version.hpp"

#include <ostream>

namespace wolfen::game {

Game::Game(std::ostream& out, std::ostream& /*err*/, engine::app::GraphicsSystem& /*graphics*/)
    : out_{&out} {}

int Game::run() {
    *out_ << "Wolfen " << engine::Version::current().text() << '\n';
    return 0;
}

} // namespace wolfen::game
