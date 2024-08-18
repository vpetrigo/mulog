cmake_minimum_required(VERSION 3.23)
project(mulog)

include(FetchContent)

set(CMAKE_C_STANDARD 11)

set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(printf_library
        GIT_REPOSITORY https://github.com/eyalroz/printf.git
        GIT_TAG v6.2.0
)
FetchContent_MakeAvailable(printf_library)

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
target_compile_options(mulog PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>)

add_library(mulog::mulog ALIAS mulog)