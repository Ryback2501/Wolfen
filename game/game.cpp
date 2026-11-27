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
constexpr Colour backgroundColour{.red = 0x64, .green = 0x95, .blue = 0xED, .alpha = 0xFF};

// The graphics APIs that can draw with the CPU, in the order runOnCpu() tries them.
constexpr std::array cpuGraphicsApis{GraphicsApi::Vulkan, GraphicsApi::Direct3D11,
                                     GraphicsApi::Direct3D12};

} // namespace

Game::Game(std::ostream& out, std::ostream& err, GraphicsSystem& graphicsSystem)
    : out_{&out}, err_{&err}, graphicsSystem_{&graphicsSystem} {}

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
    const std::array<RendererStart, 1> rendererStarts{
        [this, graphicsApi](Window& window) -> std::unique_ptr<Renderer> {
            try {
                return graphicsSystem_->startRenderer(window, graphicsApi);
            } catch (const StartupError&) {
                if (!graphicsApi) {
                    throw; // the best graphics API for this computer: the game could not start
                }
                return nullptr;
            }
        }};
    return runWith(windowMode, screen, rendererStarts, Outcome::ChosenGraphicsApiFailed);
}

Game::Outcome Game::runOnCpu(WindowMode windowMode, const Screen& screen) {
    const std::vector<GraphicsApi> availableGraphicsApis = graphicsSystem_->availableGraphicsApis();
    std::vector<RendererStart> rendererStarts;
    for (const GraphicsApi graphicsApi : cpuGraphicsApis) {
        if (std::ranges::find(availableGraphicsApis, graphicsApi) == availableGraphicsApis.end()) {
            continue;
        }
        rendererStarts.emplace_back(
            [this, graphicsApi](Window& window) -> std::unique_ptr<Renderer> {
                try {
                    return graphicsSystem_->startCpuRenderer(window, graphicsApi);
                } catch (const StartupError&) {
                    return nullptr; // no CPU device for this one: the next one is tried
                }
            });
    }
    return runWith(windowMode, screen, rendererStarts, Outcome::NoCpuGraphicsApi);
}

Game::Outcome Game::runWith(WindowMode windowMode, const Screen& screen,
                            std::span<const RendererStart> rendererStarts,
                            Outcome noRendererOutcome) {
    const std::string title = name();
    *out_ << title << '\n' << std::flush; // flushed, so it shows before the window opens
    try {
        for (const RendererStart& rendererStart : rendererStarts) {
            // Declared in this order so the renderer is destroyed before the window it draws into.
            const auto window = graphicsSystem_->openWindow({.title = title,
                                                             .windowedSize = windowSize,
                                                             .isResizable = true,
                                                             .windowMode = windowMode,
                                                             .screen = screen});
            const std::unique_ptr<Renderer> renderer = rendererStart(*window);
            if (!renderer) {
                continue; // its window closes here, before the next try opens a new one
            }
            const std::string_view softwareSuffix = renderer->drawsWithCpu() ? ", software" : "";
            window->setTitle(title + " (" + std::string{renderer->displayName()} +
                             std::string{softwareSuffix} + ")");
            play(*window, *renderer);
            screenAtQuit_ = window->screen(); // while the window is still open
            return Outcome::Ended;
        }
    } catch (const StartupError& error) {
        *err_ << "Wolfen could not start: " << error.what() << '\n';
        return Outcome::CouldNotStart;
    }
    return noRendererOutcome;
}

void Game::play(Window& window, Renderer& renderer) {
    bool isRunning = true;
    bool isVisible = true;
    while (isRunning) {
        // A hidden window is not drawn, so sleep until something happens instead of spinning.
        for (const auto event : isVisible ? window.pollEvents() : window.waitEvents()) {
            switch (event) {
            case Window::Event::CloseRequested:
            case Window::Event::EscapePressed:
                isRunning = false;
                break;
            case Window::Event::Resized:
                renderer.resize(window.pixelSize());
                break;
            case Window::Event::Hidden:
                isVisible = false;
                break;
            case Window::Event::Shown:
                isVisible = true;
                break;
            case Window::Event::FullscreenKeyPressed:
                window.switchFullscreen();
                break;
            }
        }
        if (isRunning && isVisible) {
            renderer.drawFrame(backgroundColour);
        }
    }
}

} // namespace wolfen::game
