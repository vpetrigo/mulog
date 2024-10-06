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
option(MULOG_ENABLE_DEFERRED_LOGGING "Enable deferred logging support" OFF)
option(MULOG_BUILD_EXAMPLES "Build examples" OFF)

if (MULOG_ENABLE_TESTING)
    include(CTest)
    enable_testing()
    add_compile_options(--coverage)
    add_link_options(--coverage)
    # Disable CppUTest related tests
    set(TESTS OFF CACHE BOOL "Overwrite CppUTest TESTS variable")
    set(C++11 ON CACHE BOOL "Overwrite CppUTest C++11 variable")
    add_subdirectory(external/cpputest/)
    add_subdirectory(src/)

    add_custom_target(coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR}/src . -e ".*_test.cpp"
            -e ".*main.c"
            --html-theme github.dark-blue
            --html-details coverage.html -x coverage.info)
endif ()

if (MULOG_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif ()

set(dependencies "")

if (MULOG_ENABLE_DEFERRED_LOGGING)
    FetchContent_Declare(ring_buf_library
            GIT_REPOSITORY https://github.com/MaJerle/lwrb.git
            GIT_TAG v3.2.0)
    list(APPEND dependencies ring_buf_library)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Overwrite printf BUILD_SHARED_LIBS variable")
FetchContent_Declare(printf_library
        GIT_REPOSITORY https://github.com/eyalroz/printf.git
        GIT_TAG v6.2.0)
list(APPEND dependencies printf_library)
FetchContent_MakeAvailable(${dependencies})

string(LENGTH "${MULOG_CUSTOM_CONFIG}" custom_config_path_len)

add_library(mulog
        src/color.h
        src/list.h
        src/mulog.c
        src/internal/config.h
        src/internal/interface.h
        src/internal/utils.h
        $<IF:$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>,src/internal/deferred/interface.c,src/internal/realtime/interface.c>
        include/mulog.h)
target_include_directories(mulog
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(mulog PRIVATE
        printf::printf $<$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>:lwrb>)
target_compile_definitions(mulog
        PRIVATE
        -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_OUTPUT_HANDLERS=${MULOG_OUTPUT_HANDLERS}
        PUBLIC
        $<$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>:MULOG_ENABLE_DEFERRED_LOGGING=1>)

if (custom_config_path_len GREATER 0)
    target_compile_definitions(mulog PRIVATE -DMULOG_INTERNAL_CONFIG_PATH="${MULOG_CUSTOM_CONFIG}")
endif ()

target_compile_options(mulog PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>)

add_library(mulog::mulog ALIAS mulog)