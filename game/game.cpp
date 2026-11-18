#include "game.hpp"

#include "core/colour.hpp"
#include "core/startup_error.hpp"
#include "core/version.hpp"

#include <ostream>
#include <string>

namespace wolfen::game {

namespace {

constexpr engine::Size windowSize{.width = 1280, .height = 720};
constexpr engine::Colour background{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};

} // namespace

Game::Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

int Game::run() {
    const std::string name = "Wolfen " + std::string{engine::Version::current().text()};
    *out_ << name << '\n' << std::flush; // flushed, so it shows before the window opens

    try {
        // Declared in this order so the renderer is destroyed before the window it draws into.
        const auto window =
            graphics_->openWindow({.title = name, .size = windowSize, .resizable = true});
        const auto renderer = graphics_->startRenderer(*window);
        window->setTitle(name + " (" + std::string{renderer->name()} + ")");
        play(*window, *renderer);
    } catch (const engine::StartupError& error) {
        *err_ << "Wolfen could not start: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

void Game::play(engine::platform::Window& window, engine::render::Renderer& renderer) {
    bool running = true;
    while (running) {
        for (const auto event : window.pollEvents()) {
            switch (event) {
            case engine::platform::WindowEvent::CloseRequested:
            case engine::platform::WindowEvent::EscapePressed:
                running = false;
                break;
            case engine::platform::WindowEvent::Resized:
                renderer.resize(window.pixelSize());
                break;
            }
        }
        if (running) {
            renderer.drawFrame(background);
        }
    }
}

} // namespace wolfen::game
