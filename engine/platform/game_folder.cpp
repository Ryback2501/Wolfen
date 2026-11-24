#include "platform/game_folder.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

namespace wolfen::engine::platform {

GameFolder::GameFolder() {
    // SDL gives the folder in UTF-8 (and keeps it: it is not freed here); it needs no SDL_Init.
    const char* folder = SDL_GetBasePath();
    if (folder != nullptr) {
        const std::string_view text{folder};
        // As UTF-8 characters, so a folder name outside the system's code page survives on
        // Windows.
        path_ = std::filesystem::path{std::u8string(text.begin(), text.end())};
    }
}

const std::optional<std::filesystem::path>& GameFolder::path() const {
    return path_;
}

} // namespace wolfen::engine::platform
