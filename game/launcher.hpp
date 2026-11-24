#ifndef WOLFEN_GAME_LAUNCHER_HPP
#define WOLFEN_GAME_LAUNCHER_HPP

#include "app/graphics_system.hpp"

#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace wolfen::game {

// Reads the start options and decides what to do with them: answers --help, --version and wrong
// options by itself, and starts the game only when there is a game to play.
class Launcher {
  public:
    // gameFolder: the folder that holds the game's executable, where the settings file is kept;
    // nullopt when the system cannot say where it is.
    Launcher(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics,
             const std::optional<std::filesystem::path>& gameFolder);

    // Handles the given start options (without the program's name); returns the program's exit
    // code.
    int run(std::span<const std::string_view> args);
    // The same, from the words exactly as the operating system gives them to main(): the
    // program's name first, then the start options.
    int run(int argc, char** argv);

  private:
    std::ostream* out_;
    std::ostream* err_;
    engine::app::GraphicsSystem* graphics_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_LAUNCHER_HPP
