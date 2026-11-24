#ifndef WOLFEN_GAME_GAME_HPP
#define WOLFEN_GAME_GAME_HPP

#include "app/graphics_system.hpp"
#include "platform/window.hpp"

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
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
        NoCpuRenderer,        // no renderer could start on a CPU device; nothing on stderr
    };

    Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics);

    // "Wolfen" and the version, for example "Wolfen v1.2.3".
    [[nodiscard]] static std::string name();

    // Runs the game with the given renderer, or the best one for this computer when none is given,
    // in a window or in fullscreen, on the given screen (with no name: the primary one), until it
    // ends; returns how it ended. The window is closed by then.
    Outcome run(std::optional<engine::render::Backend> backend,
                engine::platform::WindowMode windowMode, const engine::platform::Screen& screen);
    // Runs the game in a window on the primary screen, with the best renderer for this computer.
    Outcome run();
    // Runs the game like run(), but drawing with the CPU: on the first renderer, in the order
    // Vulkan, Direct3D 11, Direct3D 12 and among those this computer supports, that starts on a
    // CPU device. Ends with NoCpuRenderer, the window closed, when none does.
    Outcome runOnCpu(engine::platform::WindowMode windowMode,
                     const engine::platform::Screen& screen);

    // The screen the window was on when the player quit (its name is empty when the system gives
    // it none). Only meaningful after a run that ended with Outcome::Ended.
    [[nodiscard]] const engine::platform::Screen& screenAtQuit() const;

  private:
    // Starts a renderer for the window; null when the renderer asked for cannot start.
    using RendererStart =
        std::function<std::unique_ptr<engine::render::Renderer>(engine::platform::Window&)>;

    // Prints the start line, opens the window, starts its renderer with `startRenderer` and plays
    // until the game ends. Ends with `whenNoRenderer` when no renderer starts.
    Outcome runWith(engine::platform::WindowMode windowMode, const engine::platform::Screen& screen,
                    const RendererStart& startRenderer, Outcome whenNoRenderer);
    // The main loop: handles the window's events and draws a frame, until the game should end.
    static void play(engine::platform::Window& window, engine::render::Renderer& renderer);

    std::ostream* out_;
    std::ostream* err_;
    engine::app::GraphicsSystem* graphics_;
    engine::platform::Screen screenAtQuit_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_GAME_HPP
