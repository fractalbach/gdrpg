cmake_minimum_required(VERSION 3.20)

# GDExtension for Godot 4.x — thin wrappers around gdrpg_core.
# Enable with: cmake -DGDRPG_BUILD_EXTENSION=ON ...

option(GDRPG_GODOT_CPP_PATH "Path to godot-cpp (empty = FetchContent)" "")

if(GDRPG_GODOT_CPP_PATH AND EXISTS "${GDRPG_GODOT_CPP_PATH}/CMakeLists.txt")
    message(STATUS "Using godot-cpp from ${GDRPG_GODOT_CPP_PATH}")
    add_subdirectory(${GDRPG_GODOT_CPP_PATH} ${CMAKE_BINARY_DIR}/godot-cpp)
else()
    # Target Godot 4.7+ via godot-cpp master (v10, api_version selectable).
    # Override with -DGDRPG_GODOT_CPP_TAG=godot-4.5-stable (etc.) if needed.
    set(GDRPG_GODOT_CPP_TAG "master" CACHE STRING "godot-cpp git tag/branch")
    set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(
        godot-cpp
        GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
        GIT_TAG        ${GDRPG_GODOT_CPP_TAG}
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(godot-cpp)
endif()

add_library(gdrpg_extension SHARED
    src/register_types.cpp
    src/gdrpg_battle.cpp
    src/gdrpg_creature.cpp
)

target_include_directories(gdrpg_extension PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gdrpg_extension PRIVATE
    gdrpg::core
    godot-cpp
)

target_compile_features(gdrpg_extension PUBLIC cxx_std_20)

set_target_properties(gdrpg_extension PROPERTIES
    OUTPUT_NAME "gdrpg"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Place binaries where the Godot project expects them
set(GDRPG_EXT_BIN_DIR "${CMAKE_SOURCE_DIR}/godot_example/bin")
set_target_properties(gdrpg_extension PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${GDRPG_EXT_BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${GDRPG_EXT_BIN_DIR}"
)

if(MSVC)
    target_compile_options(gdrpg_extension PRIVATE /W4 /permissive-)
else()
    target_compile_options(gdrpg_extension PRIVATE -Wall -Wextra -Wpedantic)
endif()
