#include "core/version.hpp"

#include "core/version_text.hpp"

namespace wolfen::engine {

Version Version::current() {
    return Version{generated::versionText};
}

Version::Version(std::string_view text) : text_{text} {}

std::string_view Version::text() const {
    return text_;
}

} // namespace wolfen::engine
