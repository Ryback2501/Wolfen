#ifndef WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP
#define WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP

#include <filesystem>
#include <optional>

namespace wolfen::engine::platform {

// The folder that holds the game's executable, asked of the system when the object is built.
// Every file the game writes lives there, never in the working directory or a per-user folder, so
// the game stays portable.
class GameFolder {
  public:
    GameFolder();

    // The folder's full path; nullopt when the system cannot say where it is.
    [[nodiscard]] const std::optional<std::filesystem::path>& path() const;

  private:
    std::optional<std::filesystem::path> path_;
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP
