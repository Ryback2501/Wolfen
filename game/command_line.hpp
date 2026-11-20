#ifndef WOLFEN_GAME_COMMAND_LINE_HPP
#define WOLFEN_GAME_COMMAND_LINE_HPP

#include "render/backend.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace wolfen::game {

// The game's start options, read once from the command line.
class CommandLine {
  public:
    // What the game should do with these options.
    enum class Action : std::uint8_t { Run, Help, Version, Error };

    // args: the words after the program's name. available: the renderers this computer supports.
    CommandLine(std::span<const std::string_view> args,
                std::span<const engine::render::Backend> available);

    [[nodiscard]] Action action() const;
    // For Run: the renderer asked for with --renderer, if any.
    [[nodiscard]] std::optional<engine::render::Backend> backend() const;
    // For Error: the message to show, without the usage.
    [[nodiscard]] const std::string& error() const;

    // The usage text, listing the renderers available on this computer.
    [[nodiscard]] static std::string usage(std::span<const engine::render::Backend> available);
    // The one line shown after an option error, pointing to --help.
    [[nodiscard]] static std::string_view hint();
    // The name of a renderer as written in --renderer, for example "vulkan".
    [[nodiscard]] static std::string_view optionName(engine::render::Backend backend);

  private:
    // Records the error to report.
    void fail(std::string message);

    Action action_{Action::Run};
    std::optional<engine::render::Backend> backend_;
    std::string error_;
};

} // namespace wolfen::game

#endif // WOLFEN_GAME_COMMAND_LINE_HPP
