#include "platform/screen_list.hpp"

#include <utility>

namespace wolfen::engine::platform {

ScreenList::ScreenList(std::vector<Screen> screens) : screens_{std::move(screens)} {}

std::optional<std::size_t> ScreenList::indexOf(const Screen& saved) const {
    if (saved.name.empty()) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < screens_.size(); ++index) {
        if (screens_[index].name == saved.name) {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace wolfen::engine::platform
