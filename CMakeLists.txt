cmake_minimum_required(VERSION 3.25)

# The single source of the version is the VERSION file (vX.Y.Z).
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" WOLFEN_VERSION_RAW)
string(STRIP "${WOLFEN_VERSION_RAW}" WOLFEN_VERSION_TAG)
if(NOT WOLFEN_VERSION_TAG MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)$")
    message(FATAL_ERROR "VERSION must look like vX.Y.Z, got '${WOLFEN_VERSION_TAG}'")
endif()

project(Wolfen VERSION ${CMAKE_MATCH_1} LANGUAGES C CXX)

# Changing VERSION re-runs the configure step on the next build.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings)
include(Dependencies)
