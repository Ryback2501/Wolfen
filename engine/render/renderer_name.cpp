#include "render/renderer_name.hpp"

#include "render/graphics_api_table.hpp"

namespace wolfen::engine::render {

std::string_view RendererName::display(GraphicsApi graphicsApi, std::string_view engineName) {
    const GraphicsApiTable::Row* row = GraphicsApiTable::row(graphicsApi);
    return row == nullptr ? engineName : row->displayName;
}

} // namespace wolfen::engine::render
