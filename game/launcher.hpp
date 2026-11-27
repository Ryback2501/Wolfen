#ifndef WOLFEN_GAME_LAUNCHER_HPP
#define WOLFEN_GAME_LAUNCHER_HPP

#include "app/graphics_system.hpp"
#include "platform/screen.hpp"
#include "settings_file.hpp"

#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace wolfen::game {

// Reads the start options and decides what to do with them: answers --help, --version and wrong
// options by itself, and starts the game only when there is a game to play. A game starts on the
// screen saved in the settings file, and a normal quit saves the screen it was on.
class Launcher {
  public:
    // gameFolder: the folder that holds the game's executable, where the settings file is kept;
    // nullopt when the system cannot say where it is.
    Launcher(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphicsSystem,
             std::optional<std::filesystem::path> gameFolder);

    // Handles the given start options (without the program's name); returns the program's exit
    // code.
    int run(std::span<const std::string_view> args);
    // The same, from the words exactly as the operating system gives them to main(): the
    // program's name first, then the start options.
    int run(int argc, char** argv);

  private:
    // Saves the screen the game was on after a normal quit, or prints why it could not.
    void saveScreen(const std::optional<SettingsFile>& settingsFile,
                    const engine::platform::Screen& screen) const;

    std::ostream* out_;
    std::ostream* err_;
    engine::app::GraphicsSystem* graphicsSystem_;
    std::optional<std::filesystem::path> gameFolder_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_LAUNCHER_HPP
