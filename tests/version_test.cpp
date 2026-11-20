#include "core/version.hpp"
#include "fake_graphics.hpp"
#include "runs.hpp"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <regex>
#include <string>

namespace {

// The first line of the VERSION file, without trailing spaces or line ending.
std::string versionFileText() {
    std::ifstream file{WOLFEN_VERSION_FILE};
    std::string line;
    std::getline(file, line);
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
    return line;
}

} // namespace

using wolfen::engine::Version;
using wolfen::tests::FakeGraphicsSystem;
using wolfen::tests::GameRun;

TEST_CASE("The version is the one written in the VERSION file", "[version]") {
    REQUIRE_FALSE(versionFileText().empty());
    REQUIRE(Version::current().text() == versionFileText());
}

TEST_CASE("The version looks like vX.Y.Z", "[version]") {
    const std::string text{Version::current().text()};
    REQUIRE(std::regex_match(text, std::regex{R"(v\d+\.\d+\.\d+)"}));
}

TEST_CASE("Running the game prints 'Wolfen <version>' on exactly one line", "[game]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE(run.out() == "Wolfen " + versionFileText() + "\n");
}

TEST_CASE("Running the game ends with exit code 0", "[game]") {
    FakeGraphicsSystem graphics;
    const GameRun run{graphics};
    REQUIRE(run.exitCode() == 0);
}
