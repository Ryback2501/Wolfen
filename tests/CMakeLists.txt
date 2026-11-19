add_executable(wolfen_tests version_test.cpp game_test.cpp bgfx_renderer_test.cpp
                            launcher_test.cpp)
target_link_libraries(wolfen_tests PRIVATE game Catch2::Catch2WithMain)
# The tests compare against the real VERSION file, the single source of the version.
target_compile_definitions(wolfen_tests PRIVATE WOLFEN_VERSION_FILE="${PROJECT_SOURCE_DIR}/VERSION")
wolfen_set_warnings(wolfen_tests)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(wolfen_tests)
