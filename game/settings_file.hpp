#ifndef WOLFEN_GAME_SETTINGS_FILE_HPP
#define WOLFEN_GAME_SETTINGS_FILE_HPP

#include "platform/window.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace wolfen::game {

// The game's settings file, a plain text file of `key=value` lines. For now it holds one setting:
// the screen the game was on when it last quit.
class SettingsFile {
  public:
    explicit SettingsFile(std::filesystem::path path);

    [[nodiscard]] const std::filesystem::path& path() const;
    // The saved screen. Its name comes from the first line that starts with "screen=": everything
    // after the first '=', without spaces or a CR at the ends. The name is empty when there is
    // none: no such file, no such line, an empty name, or a file that cannot be read.
    [[nodiscard]] engine::platform::Screen screen() const;
    // Writes the file again with the given screen; false when it could not be written.
    [[nodiscard]] bool save(const engine::platform::Screen& screen) const;

  private:
    std::filesystem::path path_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_SETTINGS_FILE_HPP
