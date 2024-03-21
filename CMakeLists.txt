cmake_minimum_required(VERSION 3.26)
project(tc_steam_manager_new)

set(CMAKE_CXX_STANDARD 20)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    set(SRCS
        steam_manager.cpp
    )
    add_library(${PROJECT_NAME} ${SRCS})
    target_link_libraries(${PROJECT_NAME} tc_common_new)

elseif (APPLE)

elseif (UNIX)

endif ()