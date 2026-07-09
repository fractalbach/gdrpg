add_executable(gdrpg_demo main.cpp)

target_link_libraries(gdrpg_demo PRIVATE gdrpg::core)

target_compile_definitions(gdrpg_demo PRIVATE GDRPG_DATA_DIR="${GDRPG_DATA_DIR}")

if(MSVC)
    target_compile_options(gdrpg_demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(gdrpg_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
