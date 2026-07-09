# gdrpg_core — pure C++ combat simulation library (no Godot dependency)
add_library(gdrpg_core STATIC
    src/stats.cpp
    src/creature.cpp
    src/ability.cpp
    src/effect.cpp
    src/status_effect.cpp
    src/item.cpp
    src/persistent_entity.cpp
    src/combat_log.cpp
    src/combat_context.cpp
    src/battle.cpp
    src/data_loader.cpp
    src/registry.cpp
)

add_library(gdrpg::core ALIAS gdrpg_core)

target_include_directories(gdrpg_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(gdrpg_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_compile_features(gdrpg_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(gdrpg_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(gdrpg_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Expose data directory as a compile definition for convenience in demos/tests
target_compile_definitions(gdrpg_core
    PUBLIC
        GDRPG_DEFAULT_DATA_DIR="${GDRPG_DATA_DIR}"
)
