# Third-party code, fetched at pinned tags. Never "latest", never system packages.
include(FetchContent)

# SDL3: windowing and input. Linked statically so dist/ needs no extra libraries.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
# XTest is only used for global mouse warping; not needed, and not on the CI images.
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.20
    GIT_SHALLOW TRUE
    SYSTEM)

# bgfx, through bgfx.cmake: rendering. Declared as bgfx_cmake because bgfx.cmake itself fetches
# bgfx, bx and bimg under those names. Tools (shaderc) and examples are not needed yet.
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG v1.168.9637-616
    GIT_SHALLOW TRUE
    SYSTEM)

# Catch2 v3: unit tests.
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE
    SYSTEM)

FetchContent_MakeAvailable(SDL3 bgfx_cmake Catch2)

# bgfx also builds its Direct3D backends on Linux (for layers such as DXVK), so it reports them as
# supported there although they cannot start on a normal Linux desktop. Direct3D is for Windows.
# Setting any backend switch turns every other one off, so all the Linux backends are listed.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(bgfx PRIVATE BGFX_CONFIG_RENDERER_VULKAN=1
                                            BGFX_CONFIG_RENDERER_OPENGL=1
                                            BGFX_CONFIG_RENDERER_DIRECT3D11=0
                                            BGFX_CONFIG_RENDERER_DIRECT3D12=0)
endif()
