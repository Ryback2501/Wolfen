#include "settings_file.hpp"

#include <stdexcept>
#include <utility>

namespace wolfen::game {

SettingsFile::SettingsFile(std::filesystem::path path) : path_{std::move(path)} {}

const std::filesystem::path& SettingsFile::path() const {
    return path_;
}

std::string SettingsFile::screen() const {
    throw std::logic_error{"reading " + path_.string() + " is not written yet"};
}

bool SettingsFile::save(std::string_view /*screen*/) const {
    throw std::logic_error{"saving " + path_.string() + " is not written yet"};
}

} // namespace wolfen::game
