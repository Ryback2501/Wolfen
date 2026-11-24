#include "settings_file.hpp"
#include "temp_folder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <optional>
#include <string>

using wolfen::engine::Point;
using wolfen::engine::platform::Screen;
using wolfen::game::SettingsFile;
using wolfen::tests::TempFolder;

namespace {

// The settings file in the given folder.
SettingsFile settingsIn(const TempFolder& folder) {
    return SettingsFile{folder.path() / "settings.ini"};
}

} // namespace

TEST_CASE("No settings file: no saved screen", "[settings]") {
    const TempFolder folder;
    REQUIRE(settingsIn(folder).screen().name.empty());
}

TEST_CASE("An empty or broken file, or a screen line with no name: no saved screen", "[settings]") {
    for (const std::string text : {"", "garbage", "screen=", "screen=  \r\n", "foo=bar\n"}) {
        const TempFolder folder;
        folder.write("settings.ini", text);
        REQUIRE(settingsIn(folder).screen().name.empty());
    }
}

TEST_CASE("The saved screen is read without spaces or a CR at the ends, among other lines",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=  DP 2  \r\nfoo=bar\r\n");
    REQUIRE(settingsIn(folder).screen().name == "DP 2");

    const TempFolder otherLineFirst;
    otherLineFirst.write("settings.ini", "foo=bar\r\nscreen= HDMI-1 \r\n");
    REQUIRE(settingsIn(otherLineFirst).screen().name == "HDMI-1");
}

TEST_CASE("Only the first line that starts with screen= counts, and the name is everything after "
          "the first '='",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "myscreen=A\n screen=B\nscreen=C=D\nscreen=E\n");
    REQUIRE(settingsIn(folder).screen().name == "C=D");
}

TEST_CASE("Saving writes two lines, screen=<name> and screen_position=<x>,<y>, in place of what "
          "was there",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "foo=bar\nscreen=a much longer old name\n");
    const Screen saved{.name = "HDMI-2", .position = Point{.x = 1920, .y = 0}};
    REQUIRE(settingsIn(folder).save(saved));
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\nscreen_position=1920,0\n");
    REQUIRE(settingsIn(folder).screen() == saved);

    const TempFolder leftAndAbove;
    const Screen negative{.name = "DP-1", .position = Point{.x = -1920, .y = -200}};
    REQUIRE(settingsIn(leftAndAbove).save(negative));
    REQUIRE(leftAndAbove.text("settings.ini") == "screen=DP-1\nscreen_position=-1920,-200\n");
    REQUIRE(settingsIn(leftAndAbove).screen() == negative);
}

TEST_CASE("A screen with no name is saved as screen= and read back as no saved screen",
          "[settings]") {
    const TempFolder folder;
    REQUIRE(settingsIn(folder).save({.name = "", .position = Point{.x = 0, .y = 0}}));
    REQUIRE(folder.text("settings.ini") == "screen=\nscreen_position=0,0\n");
    REQUIRE(settingsIn(folder).screen().name.empty());
}

TEST_CASE("A screen whose position is unknown is saved with an empty screen_position= and read "
          "back with no position",
          "[settings]") {
    const TempFolder folder;
    const Screen saved{.name = "HDMI-2", .position = std::nullopt};
    REQUIRE(settingsIn(folder).save(saved));
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\nscreen_position=\n");
    REQUIRE(settingsIn(folder).screen() == saved);
}

TEST_CASE("The saved position is read with spaces around each number, and only the first "
          "screen_position= line counts",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini",
                 "screen_position= -1920 , 0 \r\nscreen=A\r\nscreen_position=5,5\n");
    REQUIRE(settingsIn(folder).screen() ==
            Screen{.name = "A", .position = Point{.x = -1920, .y = 0}});
}

TEST_CASE("A missing, empty or broken position: the screen is found by its name alone",
          "[settings]") {
    const TempFolder noPositionLine;
    noPositionLine.write("settings.ini", "screen=A\n");
    REQUIRE(settingsIn(noPositionLine).screen() == Screen{.name = "A", .position = std::nullopt});

    for (const std::string position : {"", "garbage", "+5,0", "1.5,0", "1,2,3", "1", "1,", ",2",
                                       "- 1,0", "99999999999,0", "0x10,0", "1;2"}) {
        const TempFolder folder;
        folder.write("settings.ini", "screen=A\nscreen_position=" + position + "\n");
        REQUIRE(settingsIn(folder).screen() == Screen{.name = "A", .position = std::nullopt});
    }
}

TEST_CASE("A settings.ini that is a folder: no saved screen, and saving fails", "[settings]") {
    const TempFolder folder;
    std::filesystem::create_directory(folder.path() / "settings.ini");
    REQUIRE(settingsIn(folder).screen().name.empty());
    REQUIRE_FALSE(settingsIn(folder).save({.name = "HDMI-2", .position = Point{.x = 0, .y = 0}}));
}
