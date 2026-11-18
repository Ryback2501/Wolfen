#ifndef WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP
#define WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP

#include "platform/window.hpp"

struct SDL_Window;

namespace wolfen::engine::platform {

// A window opened with SDL3. Starts SDL's video system when created and stops it when destroyed,
// so only one SdlWindow may exist at a time.
class SdlWindow final : public Window {
  public:
    // Throws StartupError when SDL cannot start or the window cannot be opened.
    explicit SdlWindow(const WindowSettings& settings);
    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) = delete;
    SdlWindow& operator=(SdlWindow&&) = delete;
    ~SdlWindow() override;

    std::vector<WindowEvent> pollEvents() override;
    std::vector<WindowEvent> waitEvents() override;
    [[nodiscard]] Size pixelSize() const override;
    void setTitle(std::string_view title) override;
    [[nodiscard]] NativeWindow nativeWindow() const override;

  private:
    SDL_Window* window_{};
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP
