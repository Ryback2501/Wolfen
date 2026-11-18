#ifndef WOLFEN_ENGINE_CORE_STARTUP_ERROR_HPP
#define WOLFEN_ENGINE_CORE_STARTUP_ERROR_HPP

#include <stdexcept>

namespace wolfen::engine {

// Thrown when part of the engine (the window, the renderer) cannot start. what() says what failed.
class StartupError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

} // namespace wolfen::engine

#endif // WOLFEN_ENGINE_CORE_STARTUP_ERROR_HPP
