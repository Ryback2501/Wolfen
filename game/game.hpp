#ifndef WOLFEN_GAME_GAME_HPP
#define WOLFEN_GAME_GAME_HPP

#include "app/graphics_system.hpp"

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

namespace wolfen::game {

// The Wolfen game. It talks to people through the two streams it is given (normal output and
// errors) and draws through the graphics system. The start options are read before it starts, by
// the launcher.
class Game {
  public:
    // How a run of the game ended.
    enum class Outcome : std::uint8_t {
        Ended,                // played until the player closed it
        ChosenRendererFailed, // the renderer it was given could not start; nothing on stderr
        CouldNotStart,        // it could not start; "Wolfen could not start: ..." was printed
    };

    Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics);

    // "Wolfen" and the version, for example "Wolfen v1.2.3".
    [[nodiscard]] static std::string name();

    // Runs the game with the given renderer, or the best one for this computer when none is given,
    // in a window or in fullscreen, until it ends; returns how it ended. The window is closed by
    // then.
    Outcome run(std::optional<engine::render::Backend> backend,
                engine::platform::WindowMode windowMode);
    // Runs the game in a window, with the best renderer for this computer.
    Outcome run();

  private:
    // The main loop: handles the window's events and draws a frame, until the game should end.
    static void play(engine::platform::Window& window, engine::render::Renderer& renderer);

    std::ostream* out_;
    std::ostream* err_;
    engine::app::GraphicsSystem* graphics_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_GAME_HPP
