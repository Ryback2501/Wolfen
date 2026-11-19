#include "launcher.hpp"

#include "command_line.hpp"
#include "game.hpp"

#include <ostream>
#include <vector>

namespace wolfen::game {

Launcher::Launcher(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

int Launcher::run(std::span<const std::string_view> args) {
    const std::vector<engine::render::Backend> available = graphics_->availableRenderers();
    const CommandLine options{args, available};

    switch (options.action()) {
    case CommandLine::Action::Help:
        *out_ << CommandLine::usage(available);
        return 0;
    case CommandLine::Action::Version:
        *out_ << Game::name() << '\n';
        return 0;
    case CommandLine::Action::Error:
        *err_ << options.error() << '\n' << CommandLine::hint() << '\n';
        return 1;
    case CommandLine::Action::Run:
        break;
    }

    Game game{*out_, *err_, *graphics_};
    return game.run(options.renderer());
}

} // namespace wolfen::game
