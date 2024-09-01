cmake_minimum_required(VERSION 3.23)
project(mulog)

include(FetchContent)
include(cmake/cpputest_utils.cmake)

set(CMAKE_C_STANDARD 11)

option(MULOG_ENABLE_TESTING "Enable tests for mulog library" OFF)
option(MULOG_ENABLE_COLOR_OUTPUT "Enable color output" OFF)
option(MULOG_ENABLE_TIMESTAMP_OUTPUT "Enable timestamp output for log entries" OFF)
set(MULOG_OUTPUT_HANDLERS 2 CACHE STRING "Maximum number of output handlers that can be registered")
set(MULOG_CUSTOM_CONFIG "" CACHE STRING "Optional path to an external config file")

if (MULOG_ENABLE_TESTING)
    include(CTest)
    enable_testing()
    add_compile_options(--coverage)
    add_link_options(--coverage)
    # Disable CppUTest related tests
    set(TESTS OFF CACHE BOOL "Overwrite CppUTest TESTS variable")
    add_subdirectory(external/cpputest/)
    add_subdirectory(src/)

    add_custom_target(coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR}/src . -e ".*_test.cpp"
            -e ".*main.c"
            --html-theme github.dark-blue
            --html-details coverage.html -x coverage.info)
endif ()

set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(printf_library
        GIT_REPOSITORY https://github.com/eyalroz/printf.git
        GIT_TAG v6.2.0
)
FetchContent_MakeAvailable(printf_library)

string(LENGTH "${MULOG_CUSTOM_CONFIG}" custom_config_path_len)

add_library(mulog
        src/color.h
        src/list.h
        src/mulog.c
        src/internal/config.h
        include/mulog.h)
target_include_directories(mulog
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(mulog PRIVATE printf::printf)
target_compile_definitions(mulog PRIVATE
        -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_OUTPUT_HANDLERS=${MULOG_OUTPUT_HANDLERS})

if (custom_config_path_len GREATER 0)
    target_compile_definitions(mulog PRIVATE -DMULOG_INTERNAL_CONFIG_PATH="${MULOG_CUSTOM_CONFIG}")
endif ()

target_compile_options(mulog PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>)

add_library(mulog::mulog ALIAS mulog)