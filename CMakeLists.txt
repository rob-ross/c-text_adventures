cmake_minimum_required(VERSION 3.28)
project(text_adventures C)

set(CMAKE_C_STANDARD 23)

include(CheckCCompilerFlag)
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
check_c_compiler_flag("-fsanitize=address" HAS_ASAN)
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=leak")
check_c_compiler_flag("-fsanitize=leak" HAS_LSAN)
unset(CMAKE_REQUIRED_LINK_OPTIONS)

# Define debug options globally for reuse
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
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
endif()

# Define external roblib dependency
add_library(roblib STATIC IMPORTED)
set_target_properties(roblib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/external/roblib/lib/libc_roblib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/roblib/include"
    # Since libc_roblib.a was built with sanitizers, anyone linking against it
    # MUST also use those sanitizer flags to pull in the required runtimes.
    INTERFACE_COMPILE_OPTIONS "$<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=leak>"
    INTERFACE_LINK_OPTIONS "$<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=leak>"
)

# Define the sources for the shared adventure library
set(ADVENTURE_LIB_SOURCES
        src/common/adventure_shared.c
        src/common/mersenne_twister.c
        src/common/console_utils.c
        src/common/parser.c
        src/common/rooms.c
        src/common/objects.c
        src/common/monsters.c
        src/common/monster_loader.c
)

# Create a static library target
add_library(adventure_lib STATIC ${ADVENTURE_LIB_SOURCES})

# Link the external roblib to our local library
target_link_libraries(adventure_lib PUBLIC roblib)

# Configure include directories for the library
# PUBLIC means these directories are used to build the library AND by anyone linking to it
# If you were to peek inside libadventure_lib.a using a terminal command like ar -t libadventure_lib.a,
# you would see a list of all the original object files that were bundled together.
target_include_directories(adventure_lib PUBLIC
        include
        src/common
)

# Apply debug options to the library
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    target_compile_options(adventure_lib PRIVATE ${DEBUG_OPTS})
endif()

function(add_adventure_executable target_name main_source macro_name)
    add_executable(${target_name} ${main_source})

    # Link the executable against our library
    target_link_libraries(${target_name} PRIVATE adventure_lib)

    target_compile_definitions(${target_name} PRIVATE ${macro_name})

    # Define the path to the monsters.json file for this adventure
    get_filename_component(target_source_dir ${main_source} DIRECTORY)
    target_compile_definitions(${target_name} PRIVATE
        MONSTER_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${target_source_dir}/monsters.json")

    if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
        target_compile_options(${target_name} PRIVATE ${DEBUG_OPTS})
        # Sanitizer flags are now automatically inherited from adventure_lib -> roblib
        # We only need to pass other link-specific debug flags if necessary.
        target_link_options(${target_name} PRIVATE -fno-omit-frame-pointer)
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


add_executable(regex_tests src/regex_tests.c)
target_link_libraries(regex_tests PRIVATE adventure_lib)
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    target_compile_options(regex_tests PRIVATE ${DEBUG_OPTS})
    target_link_options(regex_tests PRIVATE -fno-omit-frame-pointer)
endif()
