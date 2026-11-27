#include "render/graphics_api_name.hpp"

#include "render/graphics_api_table.hpp"

namespace wolfen::engine::render {

std::string_view GraphicsApiName::displayName(GraphicsApi graphicsApi,
                                              std::string_view libraryName) {
    const GraphicsApiTable::Row* row = GraphicsApiTable::row(graphicsApi);
    return row == nullptr ? libraryName : row->displayName;
}

} // namespace wolfen::engine::render
