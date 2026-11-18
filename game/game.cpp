#include "game.hpp"

#include "command_line.hpp"
#include "core/colour.hpp"
#include "core/startup_error.hpp"
#include "core/version.hpp"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace wolfen::game {

namespace {

constexpr engine::Size windowSize{.width = 1280, .height = 720};
constexpr engine::Colour background{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};

} // namespace

Game::Game(std::ostream& out, std::ostream& err, engine::app::GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

int Game::run() {
    return run({});
}

int Game::run(std::span<const std::string_view> args) {
    const std::string name = "Wolfen " + std::string{engine::Version::current().text()};
    const std::vector<engine::render::Backend> available = graphics_->availableRenderers();
    const CommandLine options{args, available};

    switch (options.action()) {
    case CommandLine::Action::Help:
        *out_ << CommandLine::usage(available);
        return 0;
    case CommandLine::Action::Version:
        *out_ << name << '\n';
        return 0;
    case CommandLine::Action::Error:
        *err_ << options.error() << '\n' << CommandLine::usage(available);
        return 1;
    case CommandLine::Action::Run:
        break;
    }

    *out_ << name << '\n' << std::flush; // flushed, so it shows before the window opens
    try {
        // Declared in this order so the renderer is destroyed before the window it draws into.
        const auto window =
            graphics_->openWindow({.title = name, .size = windowSize, .resizable = true});
        std::unique_ptr<engine::render::Renderer> renderer;
        try {
            renderer = graphics_->startRenderer(*window, options.renderer());
        } catch (const engine::StartupError&) {
            if (!options.renderer()) {
                throw;
            }
            *err_ << "Renderer '" << CommandLine::optionName(*options.renderer())
                  << "' could not be started on this computer.\n";
            return 1;
        }
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
    bool visible = true;
    while (running) {
        // A hidden window is not drawn, so sleep until something happens instead of spinning.
        for (const auto event : visible ? window.pollEvents() : window.waitEvents()) {
            switch (event) {
            case engine::platform::WindowEvent::CloseRequested:
            case engine::platform::WindowEvent::EscapePressed:
                running = false;
                break;
            case engine::platform::WindowEvent::Resized:
                renderer.resize(window.pixelSize());
                break;
            case engine::platform::WindowEvent::Hidden:
                visible = false;
                break;
            case engine::platform::WindowEvent::Shown:
                visible = true;
                break;
            }
        }
        if (running && visible) {
            renderer.drawFrame(background);
        }
    }
}

} // namespace wolfen::game
