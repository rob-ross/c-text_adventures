cmake_minimum_required(VERSION 4.2)
project(text_adventures C)

set(CMAKE_C_STANDARD 23)

include(CheckCCompilerFlag)
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
check_c_compiler_flag("-fsanitize=address" HAS_ASAN)
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=leak")
check_c_compiler_flag("-fsanitize=leak" HAS_LSAN)
unset(CMAKE_REQUIRED_LINK_OPTIONS)

function(add_adventure_executable target_name main_source macro_name)
    add_executable(${target_name}
            ${main_source}
            src/common/adventure_shared.c
            src/common/mersenne_twister.c
            src/common/console_utils.c
            src/common/parser.c
            src/common/rooms.c
            src/common/objects.c
            src/common/monsters.c
            src/common/monster_loader.c
            src/common/cu_string.c
            src/roblib/string/string_utils.c
            src/roblib/string/string_builder.c
            src/roblib/json_parser/json_parser.c
            src/roblib/json_parser/arena.c
            src/roblib/json_parser/error_result.c
    )

    target_include_directories(${target_name} PUBLIC include)
    target_compile_definitions(${target_name} PRIVATE ${macro_name})

    # Define the path to the monsters.json file for this adventure
    get_filename_component(target_source_dir ${main_source} DIRECTORY)
    target_compile_definitions(${target_name} PRIVATE
        MONSTER_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${target_source_dir}/monsters.json")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DEBUG_OPTS
            -g
            -Wall
            -Wpedantic
            -Wextra
            -Werror
            -Wno-unused-const-variable
            -Wno-unused-variable
            -Wno-unused-function
            -Wno-unused-parameter
            -Wno-sign-compare
        )
        if(HAS_ASAN)
            list(APPEND DEBUG_OPTS -fsanitize=address)
        endif()
        if(HAS_LSAN)
            list(APPEND DEBUG_OPTS -fsanitize=leak)
        endif()

        target_compile_options(${target_name} PRIVATE ${DEBUG_OPTS})
        target_link_options(${target_name} PRIVATE ${DEBUG_OPTS})
    endif()
endfunction()

add_adventure_executable(chateau_gaillard
        src/chateau_gaillard/chateau_gaillard.c
        CHATEAU_GAILLARD_MAIN)

add_adventure_executable(citadel_of_pershu
        src/citadel_of_pershu/citadel_of_pershu.c
        CITADEL_OF_PERSHU_MAIN)

add_adventure_executable(asimovian_aftermath
        src/asimovian_aftermath/asimovian_aftermath.c
        ASIMOVIAN_AFTERMATH_MAIN)

add_adventure_executable(werewolves_and_wanderer
        src/werewolves_and_wanderer/werewolves_and_wanderer.c
        WEREWOLVES_AND_WANDERER_MAIN)


add_executable(regex_tests src/regex_tests.c src/common/parser.c)
