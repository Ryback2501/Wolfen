#ifndef WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP
#define WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP

#include <filesystem>
#include <optional>

namespace wolfen::engine::platform {

// The folder that holds the game's executable. Every file the game writes lives there, never in
// the working directory or a per-user folder, so the game stays portable.
class GameFolder {
  public:
    // The folder's full path; nullopt when the system cannot say where it is.
    [[nodiscard]] static std::optional<std::filesystem::path> path();
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_GAME_FOLDER_HPP
