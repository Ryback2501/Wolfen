#ifndef WOLFEN_GAME_SETTINGS_FILE_HPP
#define WOLFEN_GAME_SETTINGS_FILE_HPP

#include "platform/screen.hpp"

#include <filesystem>

namespace wolfen::game {

// The game's settings file, a plain text file of `key=value` lines. For now it holds one setting:
// the screen the game was on when it last quit.
class SettingsFile {
  public:
    explicit SettingsFile(std::filesystem::path path);

    [[nodiscard]] const std::filesystem::path& path() const;
    // The saved screen. For each key, the first line that starts with it counts, and its value is
    // everything after the first '=', without spaces or a CR at the ends.
    // - The name comes from "screen=". It is empty when there is none: no such file, no such
    //   line, an empty name, or a file that cannot be read.
    // - The position comes from "screen_position=<x>,<y>": two whole numbers, with optional spaces
    //   around each. It is unknown (nullopt) when the line is missing, empty or anything else.
    [[nodiscard]] engine::platform::Screen screen() const;
    // Writes the file again with the given screen; false when it could not be written.
    [[nodiscard]] bool save(const engine::platform::Screen& screen) const;

  private:
    std::filesystem::path path_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_SETTINGS_FILE_HPP
