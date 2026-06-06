cmake_minimum_required(VERSION 4.2)
project(text_adventures C)

set(CMAKE_C_STANDARD 23)

function(add_adventure_executable target_name main_source macro_name)
    add_executable(${target_name}
            ${main_source}
            src/adventure_shared.c
            src/mersenne_twister.c
            src/common/console_utils.c
            src/parser.c
            src/rooms.c
            src/objects.c
            src/monsters.c
            src/common/string.c
            src/roblib/string_utils.c
    )

    target_include_directories(${target_name} PUBLIC include)
    target_compile_definitions(${target_name} PRIVATE ${macro_name})

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target_name} PRIVATE
            -g
            -fsanitize=address
            -fsanitize=leak
            -Wall
            -Werror
            -Wno-unused-const-variable
            -Wno-unused-variable
            -Wno-unused-function
        )
        target_link_options(${target_name} PRIVATE
            -fsanitize=address
            -fsanitize=leak
        )
    endif()
endfunction()

add_adventure_executable(chateau_gaillard
        src/chateau_gaillard/chateau_gaillard.c
        CHATEAU_GAILLARD_MAIN)

add_adventure_executable(citadel_of_pershu
        src/citadel_of_pershu/citadel_of_pershu.c
        CITADEL_OF_PERSHU_MAIN)


add_executable(regex_tests src/regex_tests.c)
