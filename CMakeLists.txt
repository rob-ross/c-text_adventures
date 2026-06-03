cmake_minimum_required(VERSION 4.2)
project(text_adventures C)

set(CMAKE_C_STANDARD 23)

add_executable(text_adventures
        src/chateau_gaillard/chateau_gaillard.c
        src/mersenne_twister.c
        src/common/console_utils.c
        src/parser.c
        src/rooms.c
        src/objects.c
        src/monsters.c
        src/common/string.c)

target_include_directories(text_adventures PUBLIC include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(text_adventures PRIVATE
        -DCHATEAU_GAILLARD_MAIN
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
