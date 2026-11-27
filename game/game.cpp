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
using engine::platform::WindowMode;
using engine::render::GraphicsApi;
using engine::render::Renderer;

namespace {

constexpr Size windowSize{.width = 1280, .height = 720};
constexpr Colour background{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};

// The graphics APIs that can draw with the CPU, in the order runOnCpu() tries them.
constexpr std::array cpuRenderers{GraphicsApi::Vulkan, GraphicsApi::Direct3D11,
                                  GraphicsApi::Direct3D12};

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

Game::Outcome Game::run(std::optional<GraphicsApi> graphicsApi, WindowMode windowMode,
                        const Screen& screen) {
    const std::array<RendererStart, 1> starts{
        [this, graphicsApi](Window& window) -> std::unique_ptr<Renderer> {
            try {
                return graphics_->startRenderer(window, graphicsApi);
            } catch (const StartupError&) {
                if (!graphicsApi) {
                    throw; // the best renderer for this computer: the game could not start
                }
                return nullptr;
            }
        }};
    return runWith(windowMode, screen, starts, Outcome::ChosenRendererFailed);
}

Game::Outcome Game::runOnCpu(WindowMode windowMode, const Screen& screen) {
    const std::vector<GraphicsApi> available = graphics_->availableGraphicsApis();
    std::vector<RendererStart> starts;
    for (const GraphicsApi graphicsApi : cpuRenderers) {
        if (std::ranges::find(available, graphicsApi) == available.end()) {
            continue;
        }
        starts.emplace_back([this, graphicsApi](Window& window) -> std::unique_ptr<Renderer> {
            try {
                return graphics_->startCpuRenderer(window, graphicsApi);
            } catch (const StartupError&) {
                return nullptr; // no CPU device for this one: the next one is tried
            }
        });
    }
    return runWith(windowMode, screen, starts, Outcome::NoCpuRenderer);
}

Game::Outcome Game::runWith(WindowMode windowMode, const Screen& screen,
                            std::span<const RendererStart> starts, Outcome whenNoRenderer) {
    const std::string title = name();
    *out_ << title << '\n' << std::flush; // flushed, so it shows before the window opens
    try {
        for (const RendererStart& startRenderer : starts) {
            // Declared in this order so the renderer is destroyed before the window it draws into.
            const auto window = graphics_->openWindow({.title = title,
                                                       .size = windowSize,
                                                       .resizable = true,
                                                       .mode = windowMode,
                                                       .screen = screen});
            const std::unique_ptr<Renderer> renderer = startRenderer(*window);
            if (!renderer) {
                continue; // its window closes here, before the next try opens a new one
            }
            const std::string_view cpu = renderer->drawsWithCpu() ? ", software" : "";
            window->setTitle(title + " (" + std::string{renderer->name()} + std::string{cpu} + ")");
            play(*window, *renderer);
            screenAtQuit_ = window->screen(); // while the window is still open
            return Outcome::Ended;
        }
    } catch (const StartupError& error) {
        *err_ << "Wolfen could not start: " << error.what() << '\n';
        return Outcome::CouldNotStart;
    }
    return whenNoRenderer;
}

void Game::play(Window& window, Renderer& renderer) {
    bool running = true;
    bool visible = true;
    while (running) {
        // A hidden window is not drawn, so sleep until something happens instead of spinning.
        for (const auto event : visible ? window.pollEvents() : window.waitEvents()) {
            switch (event) {
            case Window::Event::CloseRequested:
            case Window::Event::EscapePressed:
                running = false;
                break;
            case Window::Event::Resized:
                renderer.resize(window.pixelSize());
                break;
            case Window::Event::Hidden:
                visible = false;
                break;
            case Window::Event::Shown:
                visible = true;
                break;
            case Window::Event::FullscreenKeyPressed:
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
