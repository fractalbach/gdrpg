include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(gdrpg_tests
    test_stats.cpp
    test_creature.cpp
    test_battle.cpp
    test_data_loader.cpp
    test_effects.cpp
)

target_link_libraries(gdrpg_tests PRIVATE gdrpg::core Catch2::Catch2WithMain)

target_compile_definitions(gdrpg_tests PRIVATE GDRPG_DATA_DIR="${GDRPG_DATA_DIR}")

include(CTest)
include(Catch)
catch_discover_tests(gdrpg_tests)
