cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C)

option(KRSYN_BUILD_TESTS "build tests" ON)
option(KRSYN_BUILD_TOOLS "build tools" ON)

function(add_resources target dir)
    file(GLOB_RECURSE  ${target}_resources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${dir}/*)

    foreach(var ${${target}_resources})
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${var} $<TARGET_FILE_DIR:${target}>/${var})
    endforeach()
endfunction()

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
