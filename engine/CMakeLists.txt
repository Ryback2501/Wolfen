# The version text, generated from VERSION at configure time (CMake re-configures when it changes).
configure_file(core/version_text.hpp.in "${PROJECT_BINARY_DIR}/generated/engine/core/version_text.hpp"
               @ONLY)

add_library(engine STATIC core/version.cpp)
target_include_directories(engine PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(engine PRIVATE "${PROJECT_BINARY_DIR}/generated")
# PRIVATE: only engine code may use SDL3 (in platform/) and bgfx (in render/); their headers must
# not reach the game or the tests.
target_link_libraries(engine PRIVATE SDL3::SDL3-static bgfx)
wolfen_set_warnings(engine)
