#include "launcher.hpp"

namespace wolfen::game {

Launcher::Launcher(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

int Launcher::run(std::span<const std::string_view> /*args*/) {
    // Not written yet: it only asks for the available renderers, and does nothing with them.
    static_cast<void>(graphics_->availableRenderers());
    return -1;
}

} // namespace wolfen::game
