#include "game/game.hpp"

#include <iostream>

int main() {
    wolfen::game::Game game{std::cout};
    return game.run();
}
