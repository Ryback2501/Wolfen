#include "render/renderer_name.hpp"

#include "render/renderer_table.hpp"

namespace wolfen::engine::render {

std::string_view RendererName::display(GraphicsApi graphicsApi, std::string_view engineName) {
    const RendererTable::Row* row = RendererTable::find(graphicsApi);
    return row == nullptr ? engineName : row->displayName;
}

} // namespace wolfen::engine::render
