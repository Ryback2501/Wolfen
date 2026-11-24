#include "platform/game_folder.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

namespace wolfen::engine::platform {

std::optional<std::filesystem::path> GameFolder::path() {
    // SDL gives the folder in UTF-8 (and keeps it: it is not freed here); it needs no SDL_Init.
    const char* folder = SDL_GetBasePath();
    if (folder == nullptr) {
        return std::nullopt;
    }
    const std::string_view text{folder};
    // As UTF-8 characters, so a folder name outside the system's code page survives on Windows.
    return std::filesystem::path{std::u8string(text.begin(), text.end())};
}

} // namespace wolfen::engine::platform
