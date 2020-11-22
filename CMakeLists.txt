cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C)

set(BUILD_RAYGUI_EXAMPLES OFF)
set(BUILD_EXAMPLES OFF)


file(GLOB krsyn_src *.c *.h krsyn/*.c krsyn/*.h)
add_library(krsyn ${krsyn_src})

set_property(TARGET krsyn PROPERTY C_STANDARD 11)

add_executable(wav_write_test tests/wav_write_test.c)
target_link_libraries(wav_write_test krsyn)

add_executable(midi_file_test tests/midi_file_test.c)
target_link_libraries(midi_file_test krsyn)

add_executable(io_test tests/io_test.c)
target_link_libraries(io_test krsyn)

function(target_link as lib proj)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(${as} REQUIRED ${lib})
    target_include_directories(${proj} PUBLIC ${${as}_INCLUDE_DIRS})
    target_link_directories(${proj} PUBLIC ${${as}_LIBRARY_DIRS})
    target_compile_options(${proj} PUBLIC ${${as}_CFLAGS_OTHER})
    target_link_libraries(${proj} ${${as}_LIBRARIES})
endfunction()

if(NOT ${CMAKE_C_COMPILER_ID} STREQUAL TinyCC)
    add_subdirectory(raygui_editor)

    file(GLOB krsyn_editor_src editor/*.h editor/*.c)
    add_executable(krsyn_editor ${krsyn_editor_src})
    target_link(AL openal krsyn_editor)
    target_link(GTK gtk+-3.0 krsyn_editor)
    target_link_libraries(krsyn_editor krsyn)
endif()
