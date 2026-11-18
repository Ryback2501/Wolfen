#pragma once

#include <string_view>

namespace wolfen::engine {

// The project version, exactly as written in the VERSION file (for example "v0.0.1").
class Version {
  public:
    // The version this build was made from.
    static Version current();

    [[nodiscard]] std::string_view text() const;

  private:
    explicit Version(std::string_view text);

    std::string_view text_;
};

} // namespace wolfen::engine
