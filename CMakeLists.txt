cmake_minimum_required(VERSION 3.23..)
project(mulog)

include(FetchContent)

set(CMAKE_C_STANDARD 11)

option(MULOG_ENABLE_TESTING "Enable tests for mulog library" OFF)
option(MULOG_ENABLE_COLOR_OUTPUT "Enable color output" ON)
option(MULOG_ENABLE_TIMESTAMP_OUTPUT "Enable timestamp output for log entries" ON)
option(MULOG_ENABLE_LOCKING "Enable locking mechanism for multithreading/multitasking environment" ON)
set(MULOG_SINGLE_LOG_LINE_SIZE 128 CACHE STRING "Single log line maximum size")
set(MULOG_OUTPUT_HANDLERS 2 CACHE STRING "Maximum number of output handlers that can be registered")
set(MULOG_CUSTOM_CONFIG "" CACHE STRING "Optional path to an external config file")
option(MULOG_ENABLE_DEFERRED_LOGGING "Enable deferred logging support" OFF)
option(MULOG_BUILD_EXAMPLES "Build examples" OFF)
option(MULOG_INSTALL_LIBRARY "Install mulog library" OFF)

set(dependencies "")

if (MULOG_ENABLE_TESTING)
    find_program(GCOVR gcovr)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        find_program(LLVM_COV llvm-cov)
        set(GCOV_EXECUTABLE "${LLVM_COV} gcov")
        set(coverage_compile_flags -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping)
        set(coverage_link_flags -fprofile-arcs -ftest-coverage -fprofile-instr-generate)
    else ()
        find_program(GCOV gcov)
        set(GCOV_EXECUTABLE "${GCOV}")
        set(coverage_compile_flags --coverage)
        set(coverage_link_flags --coverage)
    endif ()

    macro(mulog_add_coverage_flags target)
        target_compile_options(${target} PRIVATE ${coverage_compile_flags})
        target_link_options(${target} PRIVATE ${coverage_link_flags})
    endmacro()

    include(CTest)
    include(cmake/mulog_test_utils.cmake)
    FetchContent_Declare(test_library
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.11.0
            EXCLUDE_FROM_ALL)
    FetchContent_Declare(mock_library
            GIT_REPOSITORY https://github.com/rollbear/trompeloeil.git
            GIT_TAG v49
            EXCLUDE_FROM_ALL)
    list(APPEND dependencies test_library mock_library)
    enable_testing()
    add_subdirectory(src/)

    add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E remove_directory coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory coverage
            COMMAND gcovr
                -r ${CMAKE_CURRENT_LIST_DIR}
                --gcov-executable "${GCOV_EXECUTABLE}"
                --exclude ".*_test\.cpp"
                --filter "${CMAKE_SOURCE_DIR}/src"
                --html-theme github.dark-blue
                --html-details coverage/index.html
                -x coverage.info
                --print-summary
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating coverage report with gcovr"
            VERBATIM)
endif ()

if (MULOG_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif ()

if (MULOG_ENABLE_DEFERRED_LOGGING)
    FetchContent_Declare(ring_buf_library
            GIT_REPOSITORY https://github.com/MaJerle/lwrb.git
            GIT_TAG v3.2.0
            EXCLUDE_FROM_ALL)
    list(APPEND dependencies ring_buf_library)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Overwrite printf BUILD_SHARED_LIBS variable")
FetchContent_Declare(printf_library
        GIT_REPOSITORY https://github.com/eyalroz/printf.git
        GIT_TAG v6.3.0
        EXCLUDE_FROM_ALL)
list(APPEND dependencies printf_library)
FetchContent_MakeAvailable(${dependencies})

target_compile_options(printf PRIVATE -std=c11)

string(LENGTH "${MULOG_CUSTOM_CONFIG}" custom_config_path_len)

add_library(mulog
        src/color.h
        src/list.h
        src/mulog.c
        src/internal/config.h
        src/internal/interface.h
        src/internal/utils.h
        $<IF:$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>,src/internal/deferred/interface.c,src/internal/realtime/interface.c>
        $<$<NOT:$<BOOL:${MULOG_ENABLE_LOCKING}>>:src/internal/stubs.c>
        include/mulog.h)
target_include_directories(mulog
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/>
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(mulog PRIVATE
        printf::printf $<$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>:lwrb>)
target_compile_definitions(mulog
        PRIVATE
        -DMULOG_INTERNAL_ENABLE_COLOR_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_COLOR_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT=$<IF:$<BOOL:${MULOG_ENABLE_TIMESTAMP_OUTPUT}>,1,0>
        -DMULOG_INTERNAL_OUTPUT_HANDLERS=${MULOG_OUTPUT_HANDLERS}
        -DMULOG_INTERNAL_SINGLE_LOG_LINE_SIZE=${MULOG_SINGLE_LOG_LINE_SIZE}
        -DMULOG_INTERNAL_ENABLE_LOCKING=$<IF:$<BOOL:${MULOG_ENABLE_LOCKING}>,1,0>
        PUBLIC
        $<$<BOOL:${MULOG_ENABLE_DEFERRED_LOGGING}>:MULOG_ENABLE_DEFERRED_LOGGING=1>)

if (MULOG_ENABLE_TESTING)
    mulog_add_coverage_flags(mulog)
endif ()

if (custom_config_path_len GREATER 0)
    target_compile_definitions(mulog PRIVATE -DMULOG_INTERNAL_CONFIG_PATH="${MULOG_CUSTOM_CONFIG}")
endif ()

target_compile_options(mulog PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>)

add_library(mulog::mulog ALIAS mulog)

if (MULOG_INSTALL_LIBRARY)
    include(GNUInstallDirs)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/pkg-config.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
    include(CMakePackageConfigHelpers)
    configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
            PATH_VARS CMAKE_INSTALL_LIBDIR)

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(TARGETS mulog EXPORT ${PROJECT_NAME}-targets INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT ${PROJECT_NAME}-targets NAMESPACE ${PROJECT_NAME}:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
endif ()
