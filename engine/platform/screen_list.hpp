#ifndef WOLFEN_ENGINE_PLATFORM_SCREEN_LIST_HPP
#define WOLFEN_ENGINE_PLATFORM_SCREEN_LIST_HPP

#include "platform/window.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace wolfen::engine::platform {

// The screens connected to the computer, in the order the system lists them. Finds a saved screen
// among them.
class ScreenList {
  public:
    explicit ScreenList(std::vector<Screen> screens);

    // Where the saved screen is in the list: the first screen with its name. nullopt when its name
    // is empty or no screen has it (the caller then uses the primary screen).
    [[nodiscard]] std::optional<std::size_t> indexOf(const Screen& saved) const;

  private:
    std::vector<Screen> screens_;
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_SCREEN_LIST_HPP
