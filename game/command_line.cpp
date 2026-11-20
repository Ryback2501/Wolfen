#include "command_line.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace wolfen::game {

namespace {

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

// The known renderer with this name or backend, or null. A plain loop returning a pointer, so the
// code does not depend on whether the library's array iterators are pointers.
const NamedRenderer* knownRenderer(std::string_view name) {
    for (const NamedRenderer& named : knownRenderers) {
        if (named.name == name) {
            return &named;
        }
    }
    return nullptr;
}

const NamedRenderer* knownRenderer(Backend backend) {
    for (const NamedRenderer& named : knownRenderers) {
        if (named.backend == backend) {
            return &named;
        }
    }
    return nullptr;
}

bool contains(std::span<const std::string_view> words, std::string_view word) {
    return std::ranges::find(words, word) != words.end();
}

bool contains(std::span<const Backend> backends, Backend backend) {
    return std::ranges::find(backends, backend) != backends.end();
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
        if (word != "--renderer") {
            fail("Unknown option '" + std::string{word} + "'");
            return;
        }
        if (rendererSeen) {
            fail("Option '--renderer' given twice");
            return;
        }
        rendererSeen = true;
        // A word starting with '-' is never a renderer name: it is read as an option of its own.
        if (i + 1 == args.size() || args[i + 1].starts_with('-')) {
            fail("Option '--renderer' needs a renderer name");
            return;
        }
        const std::string name{args[++i]};
        const NamedRenderer* named = knownRenderer(name);
        if (named == nullptr) {
            fail("Unknown renderer '" + name + "'. Available: " + availableList(available));
            return;
        }
        if (!contains(available, named->backend)) {
            fail("Renderer '" + name +
                 "' is not available on this computer. Available: " + availableList(available));
            return;
        }
        renderer_ = named->backend;
    }
}

void CommandLine::fail(std::string message) {
    action_ = Action::Error;
    error_ = std::move(message);
}

CommandLine::Action CommandLine::action() const {
    return action_;
}

std::optional<Backend> CommandLine::renderer() const {
    return renderer_;
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
           "  --help             Show this help and exit.\n"
           "  --version          Show the version and exit.\n"
           "\n"
           "While the game runs, press Esc to quit.\n";
}

std::string_view CommandLine::hint() {
    return "Run 'wolfen --help' to see the available options.";
}

std::string_view CommandLine::optionName(Backend backend) {
    const NamedRenderer* named = knownRenderer(backend);
    return named == nullptr ? std::string_view{} : named->name;
}

} // namespace wolfen::game
