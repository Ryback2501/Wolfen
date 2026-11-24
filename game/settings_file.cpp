#include "settings_file.hpp"

#include <cstddef>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace wolfen::game {

namespace {

// The start of the line that holds the screen's name.
constexpr std::string_view screenKey = "screen=";

// The text without spaces or CRs at either end.
std::string_view withoutSpacesAtTheEnds(std::string_view text) {
    constexpr std::string_view spaces = " \r";
    const std::size_t first = text.find_first_not_of(spaces);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

} // namespace

SettingsFile::SettingsFile(std::filesystem::path path) : path_{std::move(path)} {}

const std::filesystem::path& SettingsFile::path() const {
    return path_;
}

engine::platform::Screen SettingsFile::screen() const {
    // A file that is missing or cannot be read gives no lines.
    std::ifstream file{path_};
    std::string line;
    while (std::getline(file, line)) {
        if (line.starts_with(screenKey)) {
            return {.name = std::string{withoutSpacesAtTheEnds(
                        std::string_view{line}.substr(screenKey.size()))},
                    .position = std::nullopt};
        }
    }
    return {};
}

bool SettingsFile::save(const engine::platform::Screen& screen) const {
    std::ofstream file{path_}; // replaces what was there
    file << screenKey << screen.name << '\n';
    file.close();
    return !file.fail();
}

} // namespace wolfen::game
