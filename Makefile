# Wolfen build wrapper around CMake. Works under bash on Linux, macOS and Windows (Git Bash).
#   make deps   configure, fetch and build the third-party libraries
#   make build  build everything and stage the game into dist/
#   make test   build and run the unit tests
#   make run    build, then start the game from dist/ (pass options with ARGS="...")

SHELL := bash
BUILD_DIR := build
DIST_DIR := dist
CONFIG := Release

# Ninja on Linux and macOS; on Windows CMake picks the installed Visual Studio.
ifeq ($(OS),Windows_NT)
  GENERATOR :=
  EXE := .exe
else
  GENERATOR := -G Ninja
  EXE :=
endif

CMAKE_CONFIGURE := cmake -S . -B $(BUILD_DIR) $(GENERATOR) \
  -DCMAKE_BUILD_TYPE=$(CONFIG) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
CMAKE_BUILD := cmake --build $(BUILD_DIR) --config $(CONFIG) --parallel

.PHONY: deps build test run

deps:
	$(CMAKE_CONFIGURE)
	$(CMAKE_BUILD) --target SDL3-static bgfx Catch2WithMain

# Configure once; after that CMake re-configures itself when a CMake file or VERSION changes.
$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE_CONFIGURE)

build: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE_BUILD)
	cmake --install $(BUILD_DIR) --config $(CONFIG) --prefix $(DIST_DIR)

test: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE_BUILD)
	ctest --test-dir $(BUILD_DIR) -C $(CONFIG) --output-on-failure --no-tests=error

run: build
	./$(DIST_DIR)/wolfen$(EXE) $(ARGS)
