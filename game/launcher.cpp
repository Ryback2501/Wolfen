#include "launcher.hpp"

#include "command_line.hpp"
#include "game.hpp"

#include <optional>
#include <ostream>
#include <vector>

namespace wolfen::game {

Launcher::Launcher(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

int Launcher::run(std::span<const std::string_view> args) {
    const std::vector<engine::render::Backend> available = graphics_->availableRenderers();
    const CommandLine commandLine{args, available};

    switch (commandLine.action()) {
    case CommandLine::Action::Help:
        *out_ << CommandLine::usage(available);
        return 0;
    case CommandLine::Action::Version:
        *out_ << Game::name() << '\n';
        return 0;
    case CommandLine::Action::Error:
        *err_ << commandLine.error() << '\n' << CommandLine::hint() << '\n';
        return 1;
    case CommandLine::Action::Run:
        break;
    }

    const std::optional<engine::render::Backend> backend = commandLine.renderer();
    Game game{*out_, *err_, *graphics_};
    switch (game.run(backend)) {
    case Game::Outcome::Ended:
        return 0;
    case Game::Outcome::ChosenRendererFailed:
        *err_ << "Renderer '"
              << CommandLine::optionName(backend.value_or(engine::render::Backend::Other))
              << "' could not be started on this computer.\n";
        return 1;
    case Game::Outcome::CouldNotStart:
        break; // the game has already said why
    }
    return 1;
}

} // namespace wolfen::game
