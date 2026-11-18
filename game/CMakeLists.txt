add_executable(wolfen main.cpp)
target_link_libraries(wolfen PRIVATE engine)
wolfen_set_warnings(wolfen)

# `make build` stages the game into dist/ with `cmake --install`.
install(TARGETS wolfen RUNTIME DESTINATION .)
