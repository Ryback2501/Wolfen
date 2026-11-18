#include "command_line.hpp"

namespace wolfen::game {

CommandLine::CommandLine(std::span<const std::string_view> /*args*/,
                         std::span<const engine::render::Backend> /*available*/) {}

CommandLine::Action CommandLine::action() const {
    return action_;
}

std::optional<engine::render::Backend> CommandLine::renderer() const {
    return renderer_;
}

const std::string& CommandLine::error() const {
    return error_;
}

std::string CommandLine::usage(std::span<const engine::render::Backend> /*available*/) {
    return {};
}

std::string_view CommandLine::optionName(engine::render::Backend /*backend*/) {
    return {};
}

} // namespace wolfen::game
