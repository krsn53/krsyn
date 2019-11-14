cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()

file(GLOB krsyn_src *.c *.h)
add_library(krsyn ${krsyn_src})

add_executable(wav_write_test tests/wav_write_test.c)
target_link_libraries(wav_write_test krsyn)
target_link_libraries(wav_write_test m)

function(target_al_link proj)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AL REQUIRED openal)
    target_include_directories(${proj} PUBLIC ${AL_INCLUDE_DIRS})
    target_link_directories(${proj} PUBLIC ${AL_LIBRARY_DIRS})
    target_compile_options(${proj} PUBLIC ${AL_CFLAGS_OTHER})
    target_link_libraries(${proj} ${AL_LIBRARIES})
endfunction()


function(target_gtk_link proj)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    target_include_directories(${proj} PUBLIC ${GTK3_INCLUDE_DIRS})
    target_link_directories(${proj} PUBLIC ${GTK3_LIBRARY_DIRS})
    target_compile_options(${proj} PUBLIC ${GTK3_CFLAGS_OTHER})
    target_link_libraries(${proj} ${GTK3_LIBRARIES})
endfunction()

file(GLOB krsyn_editor_src editor/*.h editor/*.c editor/*.cpp)
add_executable(krsyn_editor ${krsyn_editor_src})
target_gtk_link(krsyn_editor)
target_al_link(krsyn_editor)
target_link_libraries(krsyn_editor krsyn)
target_link_libraries(krsyn_editor m)

add_custom_target(post ALL)

file(GLOB_RECURSE krsyn_editor_resources RELATIVE "${CMAKE_SOURCE_DIR}/editor" "editor/resource/*")
foreach(loop IN LISTS krsyn_editor_resources)
    add_custom_command(TARGET post POST_BUILD COMMAND ${CMAKE_COMMAND} -E  copy ${CMAKE_SOURCE_DIR}/editor/${loop} $<TARGET_FILE_DIR:krsyn_editor>/${loop})
endforeach(loop)

