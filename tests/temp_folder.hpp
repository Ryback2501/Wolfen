#ifndef WOLFEN_TESTS_TEMP_FOLDER_HPP
#define WOLFEN_TESTS_TEMP_FOLDER_HPP

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <string_view>

namespace wolfen::tests {

// A new, empty folder in the system's temporary folder, deleted with everything in it when the
// object goes away. It stands in for the game's folder.
class TempFolder {
  public:
    TempFolder() {
        std::random_device random;
        // A random name, again until it is one no other folder has.
        while (true) {
            path_ = std::filesystem::temp_directory_path() /
                    ("wolfen-test-" + std::to_string(random()));
            if (std::filesystem::create_directory(path_)) {
                break;
            }
        }
    }
    TempFolder(const TempFolder&) = delete;
    TempFolder& operator=(const TempFolder&) = delete;
    TempFolder(TempFolder&&) = delete;
    TempFolder& operator=(TempFolder&&) = delete;
    ~TempFolder() {
        std::error_code ignored; // a folder left behind must not fail the test run
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const {
        return path_;
    }
    // Writes a file in the folder with exactly these bytes (no line-ending translation).
    void write(std::string_view name, std::string_view text) const {
        std::ofstream file{path_ / name, std::ios::binary};
        file << text;
    }
    // The text of a file in the folder, read as text (so a line ends in "\n" on every system).
    [[nodiscard]] std::string text(std::string_view name) const {
        std::ifstream file{path_ / name};
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    [[nodiscard]] bool has(std::string_view name) const {
        return std::filesystem::exists(path_ / name);
    }

  private:
    std::filesystem::path path_;
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_TEMP_FOLDER_HPP
