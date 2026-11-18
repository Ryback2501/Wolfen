add_library(engine STATIC core/version.cpp)
target_include_directories(engine PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(engine PUBLIC SDL3::SDL3-static bgfx)
wolfen_set_warnings(engine)
