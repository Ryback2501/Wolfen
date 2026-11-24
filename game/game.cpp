#include "game.hpp"

#include "core/colour.hpp"
#include "core/startup_error.hpp"
#include "core/version.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

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

// The renderers that can draw with the CPU, in the order runOnCpu() tries them.
constexpr std::array cpuRenderers{Backend::Vulkan, Backend::Direct3D11, Backend::Direct3D12};

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
    return runWith(
        windowMode, screen,
        [this, backend](Window& window) -> std::unique_ptr<Renderer> {
            try {
                return graphics_->startRenderer(window, backend);
            } catch (const StartupError&) {
                if (!backend) {
                    throw; // the best renderer for this computer: the game could not start
                }
                return nullptr;
            }
        },
        Outcome::ChosenRendererFailed);
}

Game::Outcome Game::runOnCpu(WindowMode windowMode, const Screen& screen) {
    const std::vector<Backend> available = graphics_->availableBackends();
    return runWith(
        windowMode, screen,
        [this, &available](Window& window) -> std::unique_ptr<Renderer> {
            for (const Backend backend : cpuRenderers) {
                if (std::ranges::find(available, backend) == available.end()) {
                    continue;
                }
                try {
                    return graphics_->startCpuRenderer(window, backend);
                } catch (const StartupError&) {
                    continue; // no CPU device for this one: the next one is tried
                }
            }
            return nullptr;
        },
        Outcome::NoCpuRenderer);
}

Game::Outcome Game::runWith(WindowMode windowMode, const Screen& screen,
                            const RendererStart& startRenderer, Outcome whenNoRenderer) {
    const std::string title = name();
    *out_ << title << '\n' << std::flush; // flushed, so it shows before the window opens
    try {
        // Declared in this order so the renderer is destroyed before the window it draws into.
        const auto window = graphics_->openWindow({.title = title,
                                                   .size = windowSize,
                                                   .resizable = true,
                                                   .mode = windowMode,
                                                   .screen = screen});
        const std::unique_ptr<Renderer> renderer = startRenderer(*window);
        if (!renderer) {
            return whenNoRenderer;
        }
        const std::string_view cpu = renderer->drawsWithCpu() ? ", software" : "";
        window->setTitle(title + " (" + std::string{renderer->name()} + std::string{cpu} + ")");
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
