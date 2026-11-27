#include "platform/screen_list.hpp"

#include <utility>

namespace wolfen::engine::platform {

ScreenList::ScreenList(std::vector<Screen> screens) : screens_{std::move(screens)} {}

std::optional<std::size_t> ScreenList::indexOf(const Screen& savedScreen) const {
    if (savedScreen.name.empty()) {
        return std::nullopt;
    }
    std::optional<std::size_t> firstIndexWithName;
    for (std::size_t index = 0; index < screens_.size(); ++index) {
        const Screen& screen = screens_[index];
        if (screen.name != savedScreen.name) {
            continue;
        }
        // A screen whose position is unknown never equals a saved position.
        if (savedScreen.position && screen.position == savedScreen.position) {
            return index;
        }
        if (!firstIndexWithName) {
            firstIndexWithName = index;
        }
    }
    return firstIndexWithName;
}

} // namespace wolfen::engine::platform
