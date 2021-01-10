cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C)

option(KRSYN_BUILD_TESTS "build tests" ON)
option(KRSYN_BUILD_TOOLS "build tools" ON)

add_subdirectory(ksio)

file(GLOB krsyn_src krsyn/*.c krsyn/*.h)
add_library(krsyn ${krsyn_src})
target_include_directories(krsyn PUBLIC .)
target_link_libraries(krsyn ksio)
set_property(TARGET krsyn PROPERTY C_STANDARD 11)

if(${KRSYN_BUILD_TESTS})
    add_subdirectory(tests)
endif()


if(NOT ${CMAKE_C_COMPILER_ID} STREQUAL TinyCC AND ${KRSYN_BUILD_TOOLS})
    add_subdirectory(tools)
endif()
