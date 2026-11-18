# Wolfen build wrapper around CMake. Works under bash on Linux, macOS and Windows (Git Bash).
#   make deps   configure, fetch and build the third-party libraries
#   make build  build everything and stage the game into dist/
#   make test   build and run the unit tests
#   make run    build, then start the game from dist/ (pass options with ARGS="...")
#   make lint   check formatting (clang-format) and code (clang-tidy) of our own code
#   make fmt    format our own code in place

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

# Our own C++ files only, never third-party code.
SOURCES := $(shell find engine game tests \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null)

.PHONY: deps build test run lint fmt

deps:
	$(CMAKE_CONFIGURE)
	touch $(CONFIGURED)
	$(CMAKE_BUILD) --target SDL3-static bgfx Catch2WithMain

# Configure once; after that CMake re-configures itself when a CMake file or VERSION changes.
# The marker is written only after a configure that succeeded, so a configure that failed halfway
# (for example a network error while fetching) is run again next time.
CONFIGURED := $(BUILD_DIR)/.configured

$(CONFIGURED):
	$(CMAKE_CONFIGURE)
	touch $(CONFIGURED)

build: $(CONFIGURED)
	$(CMAKE_BUILD)
	cmake --install $(BUILD_DIR) --config $(CONFIG) --prefix $(DIST_DIR)

test: $(CONFIGURED)
	$(CMAKE_BUILD)
	ctest --test-dir $(BUILD_DIR) -C $(CONFIG) --output-on-failure --no-tests=error

run: build
	./$(DIST_DIR)/wolfen$(EXE) $(ARGS)

lint: $(CONFIGURED)
	clang-format --dry-run --Werror $(SOURCES)
	clang-tidy -p $(BUILD_DIR) --quiet $(filter %.cpp,$(SOURCES))

fmt:
	clang-format -i $(SOURCES)
