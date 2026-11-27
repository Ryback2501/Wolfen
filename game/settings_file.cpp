#include "settings_file.hpp"

#include "core/point.hpp"

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

using engine::Point;
using engine::platform::Screen;

namespace {

// The starts of the lines that hold the screen's name and its position.
constexpr std::string_view screenNameKey = "screen=";
constexpr std::string_view screenPositionKey = "screen_position=";

// The text without spaces or CRs at either end.
std::string_view withoutSpacesAtTheEnds(std::string_view text) {
    constexpr std::string_view trimmedCharacters = " \r";
    const std::size_t first = text.find_first_not_of(trimmedCharacters);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(trimmedCharacters);
    return text.substr(first, last - first + 1);
}

// A whole number with optional spaces around it: an optional '-', then digits. nullopt for
// anything else, or for a number too large for an int.
std::optional<int> wholeNumber(std::string_view text) {
    text = withoutSpacesAtTheEnds(text);
    const std::string numberText{text}; // from_chars reads characters between two plain pointers
    const char* begin = numberText.c_str();
    const char* end = std::next(begin, static_cast<std::ptrdiff_t>(numberText.size()));
    int number = 0;
    const auto [parsedEnd, errorCode] = std::from_chars(begin, end, number);
    if (errorCode != std::errc{} || parsedEnd != end) {
        return std::nullopt;
    }
    return number;
}

// The position written as "<x>,<y>", or nullopt when it is anything else.
std::optional<Point> positionFrom(std::string_view text) {
    const std::size_t commaIndex = text.find(',');
    if (commaIndex == std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<int> x = wholeNumber(text.substr(0, commaIndex));
    const std::optional<int> y = wholeNumber(text.substr(commaIndex + 1));
    if (!x || !y) {
        return std::nullopt;
    }
    return Point{.x = *x, .y = *y};
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

Screen SettingsFile::screen() const {
    // A file that is missing or cannot be read gives no lines. For each key, the first line counts.
    std::ifstream file{path_};
    Screen screen;
    bool hasNameLine = false;
    bool hasPositionLine = false;
    std::string line;
    while (std::getline(file, line)) {
        if (!hasNameLine && line.starts_with(screenNameKey)) {
            screen.name = valueAfter(screenNameKey, line);
            hasNameLine = true;
        } else if (!hasPositionLine && line.starts_with(screenPositionKey)) {
            screen.position = positionFrom(valueAfter(screenPositionKey, line));
            hasPositionLine = true;
        }
    }
    return screen;
}

bool SettingsFile::save(const Screen& screen) const {
    std::ofstream file{path_}; // replaces what was there
    file << screenNameKey << screen.name << '\n' << screenPositionKey;
    if (screen.position) {
        file << screen.position->x << ',' << screen.position->y;
    }
    file << '\n';
    file.close();
    return !file.fail();
}

} // namespace wolfen::game
