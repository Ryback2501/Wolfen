#include "settings_file.hpp"

#include <charconv>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace wolfen::game {

namespace {

// The starts of the lines that hold the screen's name and its position.
constexpr std::string_view screenKey = "screen=";
constexpr std::string_view positionKey = "screen_position=";

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

// A whole number with optional spaces around it: an optional '-', then digits. nullopt for
// anything else, or for a number too large for an int.
std::optional<int> wholeNumber(std::string_view text) {
    text = withoutSpacesAtTheEnds(text);
    const std::string digits{text}; // from_chars reads characters between two plain pointers
    const char* begin = digits.c_str();
    const char* end = std::next(begin, static_cast<std::ptrdiff_t>(digits.size()));
    int number = 0;
    const auto [stop, error] = std::from_chars(begin, end, number);
    if (error != std::errc{} || stop != end) {
        return std::nullopt;
    }
    return number;
}

// The position written as "<x>,<y>", or nullopt when it is anything else.
std::optional<engine::Point> positionFrom(std::string_view text) {
    const std::size_t comma = text.find(',');
    if (comma == std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<int> x = wholeNumber(text.substr(0, comma));
    const std::optional<int> y = wholeNumber(text.substr(comma + 1));
    if (!x || !y) {
        return std::nullopt;
    }
    return engine::Point{.x = *x, .y = *y};
}

// What follows the key at the start of the line, without spaces or a CR at the ends.
std::string_view valueAfter(std::string_view key, std::string_view line) {
    return withoutSpacesAtTheEnds(line.substr(key.size()));
}

} // namespace

SettingsFile::SettingsFile(std::filesystem::path path) : path_{std::move(path)} {}

const std::filesystem::path& SettingsFile::path() const {
    return path_;
}

engine::platform::Screen SettingsFile::screen() const {
    // A file that is missing or cannot be read gives no lines. For each key, the first line counts.
    std::ifstream file{path_};
    engine::platform::Screen screen;
    bool hasNameLine = false;
    bool hasPositionLine = false;
    std::string line;
    while (std::getline(file, line)) {
        if (!hasNameLine && line.starts_with(screenKey)) {
            screen.name = valueAfter(screenKey, line);
            hasNameLine = true;
        } else if (!hasPositionLine && line.starts_with(positionKey)) {
            screen.position = positionFrom(valueAfter(positionKey, line));
            hasPositionLine = true;
        }
    }
    return screen;
}

bool SettingsFile::save(const engine::platform::Screen& screen) const {
    std::ofstream file{path_}; // replaces what was there
    file << screenKey << screen.name << '\n' << positionKey;
    if (screen.position) {
        file << screen.position->x << ',' << screen.position->y;
    }
    file << '\n';
    file.close();
    return !file.fail();
}

} // namespace wolfen::game
