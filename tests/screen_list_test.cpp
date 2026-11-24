#include "core/point.hpp"
#include "platform/screen_list.hpp"
#include "platform/window.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>

using wolfen::engine::Point;
using wolfen::engine::platform::ScreenList;

namespace {

// Two screens of the same model side by side (same name), then another one.
const ScreenList twinsAndAnother{{
    {.name = "DELL U2720Q", .position = Point{.x = 0, .y = 0}},
    {.name = "DELL U2720Q", .position = Point{.x = 1920, .y = 0}},
    {.name = "HDMI-1", .position = Point{.x = -1280, .y = 0}},
}};

} // namespace

TEST_CASE("Two screens with the same name are told apart by their position", "[screens]") {
    REQUIRE(
        twinsAndAnother.indexOf({.name = "DELL U2720Q", .position = Point{.x = 1920, .y = 0}}) ==
        std::optional<std::size_t>{1});
    REQUIRE(twinsAndAnother.indexOf({.name = "DELL U2720Q", .position = Point{.x = 0, .y = 0}}) ==
            std::optional<std::size_t>{0});
}

TEST_CASE("A saved position no screen with that name has, or no saved position: the first screen "
          "with that name",
          "[screens]") {
    REQUIRE(twinsAndAnother.indexOf({.name = "DELL U2720Q", .position = Point{.x = 5, .y = 5}}) ==
            std::optional<std::size_t>{0});
    REQUIRE(twinsAndAnother.indexOf({.name = "DELL U2720Q", .position = std::nullopt}) ==
            std::optional<std::size_t>{0});
    // The position alone never picks a screen: the name must match too.
    REQUIRE(twinsAndAnother.indexOf({.name = "HDMI-1", .position = Point{.x = 1920, .y = 0}}) ==
            std::optional<std::size_t>{2});
}

TEST_CASE("A name no screen has, or no name: no screen (the primary one is used)", "[screens]") {
    REQUIRE_FALSE(
        twinsAndAnother.indexOf({.name = "LG 27GL850", .position = Point{.x = 0, .y = 0}}));
    REQUIRE_FALSE(twinsAndAnother.indexOf({.name = "", .position = Point{.x = 0, .y = 0}}));
}

TEST_CASE("A screen whose position the system cannot give never matches a position", "[screens]") {
    const ScreenList oneUnknown{{
        {.name = "DELL U2720Q", .position = std::nullopt},
        {.name = "DELL U2720Q", .position = Point{.x = 1920, .y = 0}},
    }};
    REQUIRE(oneUnknown.indexOf({.name = "DELL U2720Q", .position = Point{.x = 1920, .y = 0}}) ==
            std::optional<std::size_t>{1});
    REQUIRE(oneUnknown.indexOf({.name = "DELL U2720Q", .position = std::nullopt}) ==
            std::optional<std::size_t>{0});
}
