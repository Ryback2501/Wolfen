#include "platform/screen_list.hpp"

#include <utility>

namespace wolfen::engine::platform {

ScreenList::ScreenList(std::vector<Screen> screens) : screens_{std::move(screens)} {}

std::optional<std::size_t> ScreenList::indexOf(const Screen& saved) const {
    if (saved.name.empty()) {
        return std::nullopt;
    }
    std::optional<std::size_t> firstWithName;
    for (std::size_t index = 0; index < screens_.size(); ++index) {
        const Screen& screen = screens_[index];
        if (screen.name != saved.name) {
            continue;
        }
        // A screen whose position is unknown never equals a saved position.
        if (saved.position && screen.position == saved.position) {
            return index;
        }
        if (!firstWithName) {
            firstWithName = index;
        }
    }
    return firstWithName;
}

} // namespace wolfen::engine::platform
