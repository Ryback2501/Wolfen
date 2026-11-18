# The version text, generated from VERSION at configure time (CMake re-configures when it changes).
configure_file(core/version_text.hpp.in "${PROJECT_BINARY_DIR}/generated/engine/core/version_text.hpp"
               @ONLY)

add_library(engine STATIC core/version.cpp)
target_include_directories(engine PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(engine PRIVATE "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(engine PUBLIC SDL3::SDL3-static bgfx)
wolfen_set_warnings(engine)
