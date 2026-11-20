#include "render/renderer_name.hpp"

#include "render/renderer_table.hpp"

namespace wolfen::engine::render {

std::string_view RendererName::display(Backend backend, std::string_view engineName) {
    const RendererRow* row = RendererTable::find(backend);
    return row == nullptr ? engineName : row->displayName;
}

} // namespace wolfen::engine::render
