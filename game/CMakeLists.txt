# The game's logic, as a library so the unit tests can use it too.
add_library(game STATIC game.cpp)
target_link_libraries(game PUBLIC engine)
wolfen_set_warnings(game)

add_executable(wolfen main.cpp)
target_link_libraries(wolfen PRIVATE game)
wolfen_set_warnings(wolfen)

# `make build` stages the game into dist/ with `cmake --install`.
install(TARGETS wolfen RUNTIME DESTINATION .)
