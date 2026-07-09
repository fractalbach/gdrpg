cmake_minimum_required(VERSION 3.20)
project(gdrpg VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GDRPG_BUILD_DEMO "Build standalone combat demo" ON)
option(GDRPG_BUILD_TESTS "Build unit and integration tests" ON)
option(GDRPG_BUILD_EXTENSION "Build Godot GDExtension (requires godot-cpp)" OFF)

# Shared data path for demo/tests (absolute at configure time)
set(GDRPG_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Path to JSON game data")

include(FetchContent)

# nlohmann/json — header-only JSON for data loading
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(core)

if(GDRPG_BUILD_DEMO)
    add_subdirectory(demo)
endif()

if(GDRPG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GDRPG_BUILD_EXTENSION)
    add_subdirectory(extension)
endif()

message(STATUS "gdrpg ${PROJECT_VERSION}")
message(STATUS "  Demo:      ${GDRPG_BUILD_DEMO}")
message(STATUS "  Tests:     ${GDRPG_BUILD_TESTS}")
message(STATUS "  Extension: ${GDRPG_BUILD_EXTENSION}")
message(STATUS "  Data dir:  ${GDRPG_DATA_DIR}")
