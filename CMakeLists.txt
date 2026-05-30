cmake_minimum_required(VERSION 4.2)
project(text_adventures C)

set(CMAKE_C_STANDARD 23)

add_executable(text_adventures src/main.c
        src/citadel_of_pershu.c
        src/mersenne_twister.c
        src/asimovian_aftermath.c
        src/warewolves_and_wanderer.c
        src/chateau_gaillard.c
        src/common/console_utils.c
        src/parser.c
        src/rooms.c
        src/objects.c
        src/monsters.c)

target_include_directories(text_adventures PUBLIC include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(text_adventures PRIVATE
        -g
        -fsanitize=address
        -fsanitize=leak
        -Wall
        -Werror
        -Wno-unused-const-variable
        -Wno-unused-variable
        -Wno-unused-function
    )
    target_link_options(text_adventures PRIVATE
        -fsanitize=address
        -fsanitize=leak
    )
endif()

add_executable(regex_tests src/regex_tests.c)
