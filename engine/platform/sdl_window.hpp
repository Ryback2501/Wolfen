#ifndef WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP
#define WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP

#include "platform/window.hpp"

#include <optional>

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
    void switchFullscreen() override;
    [[nodiscard]] NativeWindow nativeWindow() const override;

  private:
    // Where a window was, and how big, before it went fullscreen.
    struct WindowedPlacement {
        int x{};
        int y{};
        Size size;
        bool isMaximized{};
    };

    // Saves where the window is, then covers the screen that holds its centre.
    void enterFullscreen();
    // Back to a window, where and how big it was before (where the system allows).
    void leaveFullscreen();

    SDL_Window* window_{};
    // For a window that opened in fullscreen: its size when it first leaves it.
    std::optional<Size> firstWindowedSize_;
    // Saved when F11 takes the window to fullscreen.
    std::optional<WindowedPlacement> windowedPlacement_;
    // Both are forgotten whenever the window leaves fullscreen, by F11 or by the system (a title
    // bar button, a system shortcut), so an old place is never used again. A window the system put
    // in fullscreen is put back by SDL itself.
};

} // namespace wolfen::engine::platform

#endif // WOLFEN_ENGINE_PLATFORM_SDL_WINDOW_HPP
