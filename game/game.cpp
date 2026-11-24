#include "game.hpp"

#include "core/colour.hpp"
#include "core/startup_error.hpp"
#include "core/version.hpp"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace wolfen::game {

using engine::Colour;
using engine::Size;
using engine::StartupError;
using engine::Version;
using engine::app::GraphicsSystem;
using engine::platform::Screen;
using engine::platform::Window;
using engine::platform::WindowEvent;
using engine::platform::WindowMode;
using engine::render::Backend;
using engine::render::Renderer;

namespace {

constexpr Size windowSize{.width = 1280, .height = 720};
constexpr Colour background{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};

} // namespace

Game::Game(std::ostream& out, std::ostream& err, GraphicsSystem& graphics)
    : out_{&out}, err_{&err}, graphics_{&graphics} {}

std::string Game::name() {
    return "Wolfen " + std::string{Version::current().text()};
}

Game::Outcome Game::run() {
    return run(std::nullopt, WindowMode::Windowed, {});
}

const Screen& Game::screenAtQuit() const {
    return screenAtQuit_;
}

Game::Outcome Game::run(std::optional<Backend> backend, WindowMode windowMode,
                        const Screen& screen) {
    const std::string title = name();
    *out_ << title << '\n' << std::flush; // flushed, so it shows before the window opens
    try {
        // Declared in this order so the renderer is destroyed before the window it draws into.
        const auto window = graphics_->openWindow({.title = title,
                                                   .size = windowSize,
                                                   .resizable = true,
                                                   .mode = windowMode,
                                                   .screen = screen});
        std::unique_ptr<Renderer> renderer;
        try {
            renderer = graphics_->startRenderer(*window, backend);
        } catch (const StartupError&) {
            if (!backend) {
                throw;
            }
            return Outcome::ChosenRendererFailed;
        }
        window->setTitle(title + " (" + std::string{renderer->name()} + ")");
        play(*window, *renderer);
        screenAtQuit_ = window->screen(); // while the window is still open
    } catch (const StartupError& error) {
        *err_ << "Wolfen could not start: " << error.what() << '\n';
        return Outcome::CouldNotStart;
    }
    return Outcome::Ended;
}

void Game::play(Window& window, Renderer& renderer) {
    bool running = true;
    bool visible = true;
    while (running) {
        // A hidden window is not drawn, so sleep until something happens instead of spinning.
        for (const auto event : visible ? window.pollEvents() : window.waitEvents()) {
            switch (event) {
            case WindowEvent::CloseRequested:
            case WindowEvent::EscapePressed:
                running = false;
                break;
            case WindowEvent::Resized:
                renderer.resize(window.pixelSize());
                break;
            case WindowEvent::Hidden:
                visible = false;
                break;
            case WindowEvent::Shown:
                visible = true;
                break;
            case WindowEvent::FullscreenKeyPressed:
                window.switchFullscreen();
                break;
            }
        }
        if (running && visible) {
            renderer.drawFrame(background);
        }
    }
}

} // namespace wolfen::game
