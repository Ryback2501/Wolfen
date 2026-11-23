#include "command_line.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace wolfen::game {

namespace {

using engine::platform::WindowMode;
using engine::render::Backend;

struct NamedRenderer {
    std::string_view name;
    Backend backend;
};

// Every renderer name Wolfen knows, in the fixed order used wherever a list is printed.
constexpr std::array<NamedRenderer, 5> knownRenderers{
    {{.name = "vulkan", .backend = Backend::Vulkan},
     {.name = "opengl", .backend = Backend::OpenGL},
     {.name = "metal", .backend = Backend::Metal},
     {.name = "d3d11", .backend = Backend::Direct3D11},
     {.name = "d3d12", .backend = Backend::Direct3D12}}};

// The first known renderer that `matches` accepts, or null. A plain loop returning a pointer, so
// the code does not depend on whether the library's array iterators are pointers.
template <typename Matches> const NamedRenderer* knownRenderer(const Matches& matches) {
    for (const NamedRenderer& named : knownRenderers) {
        if (matches(named)) {
            return &named;
        }
    }
    return nullptr;
}

// Whether the list holds the item. The item's type is taken from the list, so "--help" is
// compared as a string_view.
template <typename Item>
bool contains(std::span<const Item> list, const std::type_identity_t<Item>& item) {
    return std::ranges::find(list, item) != list.end();
}

// The available renderers, by name, in the fixed order: "vulkan, opengl", or "(none)".
std::string availableList(std::span<const Backend> available) {
    std::string list;
    for (const auto& named : knownRenderers) {
        if (contains(available, named.backend)) {
            list += list.empty() ? "" : ", ";
            list += named.name;
        }
    }
    return list.empty() ? "(none)" : list;
}

// The error for an option given more than once.
std::string givenTwice(std::string_view option) {
    return "Option '" + std::string{option} + "' given twice";
}

} // namespace

CommandLine::CommandLine(std::span<const std::string_view> args,
                         std::span<const Backend> available) {
    // --help wins over everything, then --version over everything else.
    if (contains(args, "--help")) {
        action_ = Action::Help;
        return;
    }
    if (contains(args, "--version")) {
        action_ = Action::Version;
        return;
    }

    // Otherwise the first error, reading left to right, is the one reported.
    bool rendererSeen = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view word = args[i];
        if (word == "--fullscreen") {
            if (windowMode_ == WindowMode::Fullscreen) {
                fail(givenTwice(word));
                return;
            }
            windowMode_ = WindowMode::Fullscreen;
            continue;
        }
        if (word != "--renderer") {
            fail("Unknown option '" + std::string{word} + "'");
            return;
        }
        if (rendererSeen) {
            fail(givenTwice(word));
            return;
        }
        rendererSeen = true;
        // A word starting with '-' is never a renderer name: it is read as an option of its own.
        if (i + 1 == args.size() || args[i + 1].starts_with('-')) {
            fail("Option '--renderer' needs a renderer name");
            return;
        }
        const std::string name{args[++i]};
        const NamedRenderer* named = knownRenderer(
            [&name](const NamedRenderer& candidate) { return candidate.name == name; });
        if (named == nullptr || !contains(available, named->backend)) {
            const std::string problem =
                named == nullptr ? "Unknown renderer '" + name + "'"
                                 : "Renderer '" + name + "' is not available on this computer";
            fail(problem + ". Available: " + availableList(available));
            return;
        }
        backend_ = named->backend;
    }
}

void CommandLine::fail(std::string message) {
    action_ = Action::Error;
    error_ = std::move(message);
}

CommandLine::Action CommandLine::action() const {
    return action_;
}

std::optional<Backend> CommandLine::backend() const {
    return backend_;
}

WindowMode CommandLine::windowMode() const {
    return windowMode_;
}

const std::string& CommandLine::error() const {
    return error_;
}

std::string CommandLine::usage(std::span<const Backend> available) {
    return "Usage: wolfen [options]\n"
           "\n"
           "Options:\n"
           "  --renderer <name>  Draw with this renderer. Available on this computer: " +
           availableList(available) +
           "\n"
           "  --fullscreen       Start in borderless fullscreen.\n"
           "  --help             Show this help and exit.\n"
           "  --version          Show the version and exit.\n";
}

std::string_view CommandLine::hint() {
    return "Run 'wolfen --help' to see the available options.";
}

std::string_view CommandLine::optionName(Backend backend) {
    const NamedRenderer* named = knownRenderer(
        [backend](const NamedRenderer& candidate) { return candidate.backend == backend; });
    return named == nullptr ? std::string_view{} : named->name;
}

} // namespace wolfen::game
