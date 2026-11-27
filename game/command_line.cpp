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

using engine::platform::WindowMode;
using engine::render::GraphicsApi;

namespace {

// What a start option does once it is read.
enum class Option : std::uint8_t { Renderer, Fullscreen, Software, Help, Version };

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
constexpr std::array<OptionRow, 5> options{{
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
    {.option = Option::Software,
     .name = "--software",
     .valueShown = "",
     .valueNeeded = "",
     .help = "Draw with the CPU instead of the graphics card."},
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

// Whether the option takes a value (the word after it).
constexpr bool takesValue(const OptionRow& row) {
    return !row.valueShown.empty();
}

// An option that takes a value names it both in the usage and in its error.
static_assert(std::ranges::all_of(options,
                                  [](const OptionRow& row) {
                                      return takesValue(row) == !row.valueNeeded.empty();
                                  }),
              "an option's value is described both for the usage and for its error, or not at all");

// An option as the usage shows it: its name, then its value if it takes one.
std::string shownOption(const OptionRow& row) {
    return takesValue(row) ? std::string{row.name} + " " + std::string{row.valueShown}
                           : std::string{row.name};
}

// A graphics API and its name in the --renderer option. Players say "renderer" (the option, the
// help and the messages); the code calls it GraphicsApi.
struct NamedGraphicsApi {
    std::string_view name;
    GraphicsApi graphicsApi;
};

// Every graphics API name Wolfen knows, in the fixed order used wherever a list is printed.
constexpr std::array<NamedGraphicsApi, 5> knownGraphicsApis{
    {{.name = "vulkan", .graphicsApi = GraphicsApi::Vulkan},
     {.name = "opengl", .graphicsApi = GraphicsApi::OpenGL},
     {.name = "metal", .graphicsApi = GraphicsApi::Metal},
     {.name = "d3d11", .graphicsApi = GraphicsApi::Direct3D11},
     {.name = "d3d12", .graphicsApi = GraphicsApi::Direct3D12}}};

// The first row of the table that `matches` accepts, or null. A plain loop returning a pointer, so
// the code does not depend on whether the library's array iterators are pointers.
template <typename Row, std::size_t rowCount, typename Matches>
constexpr const Row* firstRow(const std::array<Row, rowCount>& table, const Matches& matches) {
    for (const Row& row : table) {
        if (matches(row)) {
            return &row;
        }
    }
    return nullptr;
}

// The help option's row: the hint line after an error names it. Worked out while compiling: without
// a help row there is nothing to follow, and the build fails here.
constexpr const OptionRow& helpRow =
    *firstRow(options, [](const OptionRow& row) { return row.option == Option::Help; });

// The software option's row: with it, the renderer option is ignored. Worked out while compiling,
// like the help row.
constexpr const OptionRow& softwareRow =
    *firstRow(options, [](const OptionRow& row) { return row.option == Option::Software; });

// Whether the list holds the item.
template <typename List>
bool contains(const List& list, const std::ranges::range_value_t<List>& item) {
    return std::ranges::find(list, item) != std::ranges::end(list);
}

// The available graphics APIs, by name, in the fixed order: "vulkan, opengl", or "(none)".
std::string availableGraphicsApiNames(std::span<const GraphicsApi> availableGraphicsApis) {
    std::string list;
    for (const auto& namedGraphicsApi : knownGraphicsApis) {
        if (contains(availableGraphicsApis, namedGraphicsApi.graphicsApi)) {
            list += list.empty() ? "" : ", ";
            list += namedGraphicsApi.name;
        }
    }
    return list.empty() ? "(none)" : list;
}

} // namespace

// The software option is read first, because it changes how the renderer option is read wherever
// that comes. It can never be another option's value, as it starts with '-'.
CommandLine::CommandLine(std::span<const std::string_view> args,
                         std::span<const GraphicsApi> availableGraphicsApis)
    : isSoftwareAsked_{contains(args, softwareRow.name)} {
    // Every word is read, and only the first error, reading left to right, is kept. The help
    // option, then the version option, win over everything else wherever they are.
    std::optional<Action> action;
    std::vector<Option> seen;
    // Whether a value follows the word at `at`. A word starting with '-' is never a value: it is
    // read as an option of its own.
    const auto valueFollows = [args](std::size_t at) {
        return at + 1 < args.size() && !args[at + 1].starts_with('-');
    };
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view word = args[i];
        const OptionRow* row = firstRow(
            options, [word](const OptionRow& candidate) { return candidate.name == word; });
        if (row == nullptr) {
            fail("Unknown option '" + std::string{word} + "'");
            continue;
        }
        if (row->option == Option::Renderer && isSoftwareAsked_) {
            // Ignored completely, with its value if one follows: nothing about it is checked.
            if (valueFollows(i)) {
                ++i;
            }
            continue;
        }
        if (contains(seen, row->option)) {
            fail("Option '" + std::string{word} + "' given twice");
            if (takesValue(*row) && valueFollows(i)) {
                ++i; // its value is not an option of its own
            }
            continue;
        }
        seen.push_back(row->option);

        std::string_view value;
        if (takesValue(*row)) {
            if (!valueFollows(i)) {
                fail("Option '" + std::string{word} + "' needs " + std::string{row->valueNeeded});
                continue;
            }
            value = args[++i];
        }

        switch (row->option) {
        case Option::Renderer:
            readGraphicsApi(value, availableGraphicsApis);
            break;
        case Option::Fullscreen:
            windowMode_ = WindowMode::Fullscreen;
            break;
        case Option::Software:
            break; // already read before the loop
        case Option::Help:
            action = Action::Help;
            break;
        case Option::Version:
            action = action.value_or(Action::Version); // the help option still wins
            break;
        }
    }

    if (action) {
        action_ = *action;
    } else if (!error_.empty()) {
        action_ = Action::Error;
    }
}

void CommandLine::readGraphicsApi(std::string_view name,
                                  std::span<const GraphicsApi> availableGraphicsApis) {
    const NamedGraphicsApi* namedGraphicsApi =
        firstRow(knownGraphicsApis,
                 [name](const NamedGraphicsApi& candidate) { return candidate.name == name; });
    if (namedGraphicsApi == nullptr ||
        !contains(availableGraphicsApis, namedGraphicsApi->graphicsApi)) {
        const std::string problem =
            namedGraphicsApi == nullptr
                ? "Unknown renderer '" + std::string{name} + "'"
                : "Renderer '" + std::string{name} + "' is not available on this computer";
        fail(problem + ". Available: " + availableGraphicsApiNames(availableGraphicsApis));
        return;
    }
    graphicsApi_ = namedGraphicsApi->graphicsApi;
}

void CommandLine::fail(std::string message) {
    if (error_.empty()) {
        error_ = std::move(message);
    }
}

CommandLine::Action CommandLine::action() const {
    return action_;
}

std::optional<GraphicsApi> CommandLine::graphicsApi() const {
    return graphicsApi_;
}

bool CommandLine::isSoftwareAsked() const {
    return isSoftwareAsked_;
}

WindowMode CommandLine::windowMode() const {
    return windowMode_;
}

const std::string& CommandLine::error() const {
    return error_;
}

std::string CommandLine::usage(std::span<const GraphicsApi> availableGraphicsApis) {
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
            text += " " + availableGraphicsApiNames(availableGraphicsApis);
        }
        text += '\n';
    }
    return text;
}

std::string CommandLine::hint() {
    return "Run 'wolfen " + std::string{helpRow.name} + "' to see the available options.";
}

std::string_view CommandLine::graphicsApiName(GraphicsApi graphicsApi) {
    const NamedGraphicsApi* namedGraphicsApi =
        firstRow(knownGraphicsApis, [graphicsApi](const NamedGraphicsApi& candidate) {
            return candidate.graphicsApi == graphicsApi;
        });
    return namedGraphicsApi == nullptr ? std::string_view{} : namedGraphicsApi->name;
}

} // namespace wolfen::game
