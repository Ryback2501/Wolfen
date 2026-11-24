#include "platform/game_folder.hpp"

namespace wolfen::engine::platform {

std::optional<std::filesystem::path> GameFolder::path() {
    return std::nullopt;
}

} // namespace wolfen::engine::platform
