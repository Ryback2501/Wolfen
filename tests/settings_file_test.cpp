#include "settings_file.hpp"
#include "temp_folder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

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
    REQUIRE(settingsIn(folder).screen().empty());
}

TEST_CASE("An empty or broken file, or a screen line with no name: no saved screen", "[settings]") {
    for (const std::string text : {"", "garbage", "screen=", "screen=  \r\n", "foo=bar\n"}) {
        const TempFolder folder;
        folder.write("settings.ini", text);
        REQUIRE(settingsIn(folder).screen().empty());
    }
}

TEST_CASE("The saved screen is read without spaces or a CR at the ends, among other lines",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "screen=  DP 2  \r\nfoo=bar\r\n");
    REQUIRE(settingsIn(folder).screen() == "DP 2");

    const TempFolder otherLineFirst;
    otherLineFirst.write("settings.ini", "foo=bar\r\nscreen= HDMI-1 \r\n");
    REQUIRE(settingsIn(otherLineFirst).screen() == "HDMI-1");
}

TEST_CASE("Only the first line that starts with screen= counts, and the name is everything after "
          "the first '='",
          "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "myscreen=A\n screen=B\nscreen=C=D\nscreen=E\n");
    REQUIRE(settingsIn(folder).screen() == "C=D");
}

TEST_CASE("Saving writes one line, screen=<name>, in place of what was there", "[settings]") {
    const TempFolder folder;
    folder.write("settings.ini", "foo=bar\nscreen=a much longer old name\n");
    REQUIRE(settingsIn(folder).save("HDMI-2"));
    REQUIRE(folder.text("settings.ini") == "screen=HDMI-2\n");
    REQUIRE(settingsIn(folder).screen() == "HDMI-2");
}

TEST_CASE("A screen with no name is saved as screen= and read back as no saved screen",
          "[settings]") {
    const TempFolder folder;
    REQUIRE(settingsIn(folder).save(""));
    REQUIRE(folder.text("settings.ini") == "screen=\n");
    REQUIRE(settingsIn(folder).screen().empty());
}

TEST_CASE("A settings.ini that is a folder: no saved screen, and saving fails", "[settings]") {
    const TempFolder folder;
    std::filesystem::create_directory(folder.path() / "settings.ini");
    REQUIRE(settingsIn(folder).screen().empty());
    REQUIRE_FALSE(settingsIn(folder).save("HDMI-2"));
}
