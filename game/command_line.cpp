#include "command_line.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace wolfen::game {

namespace {

using engine::platform::WindowMode;
using engine::render::Backend;

// What a start option does once it is read.
enum class Option : std::uint8_t { Renderer, Fullscreen, Help, Version };

// One start option: everything the reading and the usage need to know about it.
struct OptionRow {
    Option option;
    std::string_view name;
    // The option's value as the usage shows it ("<name>"); empty for an option with no value.
    std::string_view valueShown;
    // The option's value as its error names it ("a renderer name").
    std::string_view valueNeeded;
    // The usage's help line.
    std::string_view help;
};

// Every start option, in the order the usage lists them.
constexpr std::array<OptionRow, 4> options{{
    {.option = Option::Renderer,
     .name = "--renderer",
     .valueShown = "<name>",
     .valueNeeded = "a renderer name",
     .help = "Draw with this renderer. Available on this computer:"},
    {.option = Option::Fullscreen,
     .name = "--fullscreen",
     .valueShown = "",
     .valueNeeded = "",
     .help = "Start in borderless fullscreen."},
    {.option = Option::Help,
     .name = "--help",
     .valueShown = "",
     .valueNeeded = "",
     .help = "Show this help and exit."},
    {.option = Option::Version,
     .name = "--version",
     .valueShown = "",
     .valueNeeded = "",
     .help = "Show the version and exit."},
}};

// An option as the usage shows it: its name, then its value if it takes one ("--renderer <name>").
std::string shownOption(const OptionRow& row) {
    return row.valueShown.empty() ? std::string{row.name}
                                  : std::string{row.name} + " " + std::string{row.valueShown};
}

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

// The first row of the table that `matches` accepts, or null. A plain loop returning a pointer, so
// the code does not depend on whether the library's array iterators are pointers.
template <typename Row, std::size_t rowCount, typename Matches>
const Row* firstRow(const std::array<Row, rowCount>& table, const Matches& matches) {
    for (const Row& row : table) {
        if (matches(row)) {
            return &row;
        }
    }
    return nullptr;
}

// Whether the list holds the item.
template <typename List>
bool contains(const List& list, const std::ranges::range_value_t<List>& item) {
    return std::ranges::find(list, item) != std::ranges::end(list);
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
    // Every word is read, and only the first error, reading left to right, is kept: --help, then
    // --version, win over everything else wherever they are.
    std::optional<Action> answer;
    std::vector<Option> seen;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view word = args[i];
        const OptionRow* row = firstRow(
            options, [word](const OptionRow& candidate) { return candidate.name == word; });
        if (row == nullptr) {
            fail("Unknown option '" + std::string{word} + "'");
            continue;
        }
        if (contains(seen, row->option)) {
            fail("Option '" + std::string{word} + "' given twice");
            continue;
        }
        seen.push_back(row->option);

        std::string_view value;
        if (!row->valueShown.empty()) {
            // A word starting with '-' is never a value: it is read as an option of its own.
            if (i + 1 == args.size() || args[i + 1].starts_with('-')) {
                fail("Option '" + std::string{word} + "' needs " + std::string{row->valueNeeded});
                continue;
            }
            value = args[++i];
        }

        switch (row->option) {
        case Option::Renderer:
            readRenderer(value, available);
            break;
        case Option::Fullscreen:
            windowMode_ = WindowMode::Fullscreen;
            break;
        case Option::Help:
            answer = Action::Help;
            break;
        case Option::Version:
            answer = answer.value_or(Action::Version); // --help still wins
            break;
        }
    }

    if (answer) {
        action_ = *answer;
    } else if (!error_.empty()) {
        action_ = Action::Error;
    }
}

void CommandLine::readRenderer(std::string_view name, std::span<const Backend> available) {
    const NamedRenderer* named = firstRow(
        knownRenderers, [name](const NamedRenderer& candidate) { return candidate.name == name; });
    if (named == nullptr || !contains(available, named->backend)) {
        const std::string problem =
            named == nullptr
                ? "Unknown renderer '" + std::string{name} + "'"
                : "Renderer '" + std::string{name} + "' is not available on this computer";
        fail(problem + ". Available: " + availableList(available));
        return;
    }
    backend_ = named->backend;
}

void CommandLine::fail(std::string message) {
    if (error_.empty()) {
        error_ = std::move(message);
    }
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
    // The help lines start two spaces after the longest option as shown.
    std::size_t helpColumn = 0;
    for (const OptionRow& row : options) {
        helpColumn = std::max(helpColumn, shownOption(row).size() + 2);
    }

    std::string text = "Usage: wolfen [options]\n"
                       "\n"
                       "Options:\n";
    for (const OptionRow& row : options) {
        const std::string shown = shownOption(row);
        text += "  " + shown + std::string(helpColumn - shown.size(), ' ') + std::string{row.help};
        if (row.option == Option::Renderer) {
            text += " " + availableList(available);
        }
        text += '\n';
    }
    return text;
}

std::string_view CommandLine::hint() {
    return "Run 'wolfen --help' to see the available options.";
}

std::string_view CommandLine::optionName(Backend backend) {
    const NamedRenderer* named =
        firstRow(knownRenderers, [backend](const NamedRenderer& candidate) {
            return candidate.backend == backend;
        });
    return named == nullptr ? std::string_view{} : named->name;
}

} // namespace wolfen::game
