#ifndef WOLFEN_ENGINE_RENDER_RENDERER_TABLE_HPP
#define WOLFEN_ENGINE_RENDER_RENDERER_TABLE_HPP

// Included only by .cpp files in render/: it names bgfx types, and only render/ may use bgfx.

#include "render/renderer_name.hpp"

#include <bgfx/bgfx.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace wolfen::engine::render {

// One renderer Wolfen names: its backend, bgfx's renderer type for it, and its name as shown to
// people.
struct RendererRow {
    Backend backend;
    bgfx::RendererType::Enum bgfxType;
    std::string_view displayName;
};

// Every renderer Wolfen names, one row each, so each renderer's details are written once.
class RendererTable {
  public:
    // The row for a backend, or null for Backend::Other.
    static constexpr const RendererRow* find(Backend backend) {
        return findRow([backend](const RendererRow& row) { return row.backend == backend; });
    }

    // The row for a bgfx renderer type, or null for a type Wolfen does not name.
    static constexpr const RendererRow* find(bgfx::RendererType::Enum bgfxType) {
        return findRow([bgfxType](const RendererRow& row) { return row.bgfxType == bgfxType; });
    }

  private:
    // In the order of Backend, one row for every value before Other.
    static constexpr std::array<RendererRow, 5> rows_{
        {{.backend = Backend::Vulkan,
          .bgfxType = bgfx::RendererType::Vulkan,
          .displayName = "Vulkan"},
         {.backend = Backend::OpenGL,
          .bgfxType = bgfx::RendererType::OpenGL,
          .displayName = "OpenGL"},
         {.backend = Backend::Metal, .bgfxType = bgfx::RendererType::Metal, .displayName = "Metal"},
         {.backend = Backend::Direct3D11,
          .bgfxType = bgfx::RendererType::Direct3D11,
          .displayName = "Direct3D 11"},
         {.backend = Backend::Direct3D12,
          .bgfxType = bgfx::RendererType::Direct3D12,
          .displayName = "Direct3D 12"}}};

    // The first row that `matches` accepts, or null. A plain loop returning a pointer, so the code
    // does not depend on whether the library's array iterators are pointers.
    template <typename Matches>
    static constexpr const RendererRow* findRow(const Matches& matches) {
        for (const RendererRow& row : rows_) {
            if (matches(row)) {
                return &row;
            }
        }
        return nullptr;
    }

    // A Backend value added before Other without its row (or rows out of order) stops the build.
    static_assert(
        [] {
            if (rows_.size() != static_cast<std::size_t>(Backend::Other)) {
                return false;
            }
            for (std::size_t i = 0; i < rows_.size(); ++i) {
                if (static_cast<std::size_t>(rows_.at(i).backend) != i) {
                    return false;
                }
            }
            return true;
        }(),
        "RendererTable needs one row for every Backend value before Other, in Backend's order "
        "(engine/render/renderer_table.hpp)");
};

} // namespace wolfen::engine::render

#endif // WOLFEN_ENGINE_RENDER_RENDERER_TABLE_HPP
