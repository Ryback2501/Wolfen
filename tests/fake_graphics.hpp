#ifndef WOLFEN_TESTS_FAKE_GRAPHICS_HPP
#define WOLFEN_TESTS_FAKE_GRAPHICS_HPP

#include "app/graphics_system.hpp"
#include "core/colour.hpp"
#include "core/size.hpp"
#include "core/startup_error.hpp"

#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace wolfen::tests {

// What happened to the fake window and renderer, for the tests to check. It outlives them.
struct GraphicsRecord {
    std::vector<std::string> steps; // "open window", "start renderer", "close renderer", ...
    engine::platform::WindowSettings windowSettings; // as asked for by the last openWindow()
    std::vector<std::string> titles;
    std::vector<engine::Colour> frames;
    std::vector<engine::Size> resizes;
    std::optional<engine::render::Backend> requestedBackend; // as asked of the last startRenderer()
    int waits{};                        // how many times the game slept until the window had events
    std::string outputWhenWindowOpened; // what the game had printed when it opened the window
};

// A window with no screen: it hands out the events it was given, one batch per poll, and asks to
// close once they run out so a test can never loop forever.
class FakeWindow final : public engine::platform::Window {
  public:
    FakeWindow(GraphicsRecord& record,
               std::deque<std::vector<engine::platform::WindowEvent>> events,
               engine::Size pixelSize, engine::Size sizeAfterResize)
        : record_{&record}, events_{std::move(events)}, pixelSize_{pixelSize},
          sizeAfterResize_{sizeAfterResize} {}
    FakeWindow(const FakeWindow&) = delete;
    FakeWindow& operator=(const FakeWindow&) = delete;
    FakeWindow(FakeWindow&&) = delete;
    FakeWindow& operator=(FakeWindow&&) = delete;
    ~FakeWindow() override {
        record_->steps.emplace_back("close window");
    }

    std::vector<engine::platform::WindowEvent> pollEvents() override {
        if (events_.empty()) {
            return {engine::platform::WindowEvent::CloseRequested};
        }
        auto batch = std::move(events_.front());
        events_.pop_front();
        for (const auto event : batch) {
            if (event == engine::platform::WindowEvent::Resized) {
                pixelSize_ = sizeAfterResize_;
            }
        }
        return batch;
    }
    std::vector<engine::platform::WindowEvent> waitEvents() override {
        ++record_->waits;
        return pollEvents();
    }
    [[nodiscard]] engine::Size pixelSize() const override {
        return pixelSize_;
    }
    void setTitle(std::string_view title) override {
        record_->titles.emplace_back(title);
    }
    [[nodiscard]] engine::platform::NativeWindow nativeWindow() const override {
        return {};
    }

  private:
    GraphicsRecord* record_;
    std::deque<std::vector<engine::platform::WindowEvent>> events_;
    engine::Size pixelSize_;
    engine::Size sizeAfterResize_;
};

// A renderer that draws nothing and records every frame.
class FakeRenderer final : public engine::render::Renderer {
  public:
    FakeRenderer(GraphicsRecord& record, std::string name)
        : record_{&record}, name_{std::move(name)} {}
    FakeRenderer(const FakeRenderer&) = delete;
    FakeRenderer& operator=(const FakeRenderer&) = delete;
    FakeRenderer(FakeRenderer&&) = delete;
    FakeRenderer& operator=(FakeRenderer&&) = delete;
    ~FakeRenderer() override {
        record_->steps.emplace_back("close renderer");
    }

    [[nodiscard]] std::string_view name() const override {
        return name_;
    }
    void resize(engine::Size pixelSize) override {
        record_->resizes.push_back(pixelSize);
    }
    void drawFrame(engine::Colour background) override {
        record_->frames.push_back(background);
    }

  private:
    GraphicsRecord* record_;
    std::string name_;
};

// A graphics system with no screen. Each test sets what it should do, then runs the game with it.
class FakeGraphicsSystem final : public engine::app::GraphicsSystem {
  public:
    GraphicsRecord record;
    std::deque<std::vector<engine::platform::WindowEvent>> events; // one batch per poll
    engine::Size pixelSize{.width = 1280, .height = 720};          // when the window opens
    engine::Size sizeAfterResize{.width = 1280, .height = 720}; // from the first Resized event on
    std::string rendererName{"Fake"};
    std::optional<std::string> windowFailure;   // when set, openWindow() fails with it
    std::optional<std::string> rendererFailure; // when set, startRenderer() fails with it
    std::vector<engine::render::Backend> available{engine::render::Backend::Vulkan,
                                                   engine::render::Backend::OpenGL};
    const std::ostringstream* watchedOutput{}; // when set, recorded when the window opens

    [[nodiscard]] std::vector<engine::render::Backend> availableBackends() const override {
        return available;
    }

    std::unique_ptr<engine::platform::Window>
    openWindow(const engine::platform::WindowSettings& settings) override {
        record.steps.emplace_back("open window");
        record.windowSettings = settings;
        if (watchedOutput != nullptr) {
            record.outputWhenWindowOpened = watchedOutput->str();
        }
        if (windowFailure) {
            throw engine::StartupError{*windowFailure};
        }
        return std::make_unique<FakeWindow>(record, std::move(events), pixelSize, sizeAfterResize);
    }

    std::unique_ptr<engine::render::Renderer>
    startRenderer(engine::platform::Window& /*window*/,
                  std::optional<engine::render::Backend> backend) override {
        record.requestedBackend = backend;
        record.steps.emplace_back("start renderer");
        if (rendererFailure) {
            throw engine::StartupError{*rendererFailure};
        }
        return std::make_unique<FakeRenderer>(record, rendererName);
    }
};

} // namespace wolfen::tests

#endif // WOLFEN_TESTS_FAKE_GRAPHICS_HPP
