cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C)


if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()

file(GLOB krsyn_src *.c *.h)
add_library(krsyn ${krsyn_src})

add_executable(wav_write_test tests/wav_write_test.c)
target_link_libraries(wav_write_test krsyn)

function(target_link as lib proj)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(${as} REQUIRED ${lib})
    target_include_directories(${proj} PUBLIC ${${as}_INCLUDE_DIRS})
    target_link_directories(${proj} PUBLIC ${${as}_LIBRARY_DIRS})
    target_compile_options(${proj} PUBLIC ${${as}_CFLAGS_OTHER})
    target_link_libraries(${proj} ${${as}_LIBRARIES})
endfunction()

file(GLOB krsyn_editor_src editor/*.h editor/*.c editor/*.cpp)
add_executable(krsyn_editor ${krsyn_editor_src})
target_link(AL openal krsyn_editor)
target_link(GTK gtk+-3.0 krsyn_editor)
target_link_libraries(krsyn_editor krsyn)
target_link_libraries(krsyn_editor m)

add_custom_target(post ALL)

file(GLOB_RECURSE krsyn_editor_resources RELATIVE "${CMAKE_SOURCE_DIR}/editor" "editor/resource/*")
foreach(loop IN LISTS krsyn_editor_resources)
    add_custom_command(TARGET post POST_BUILD COMMAND ${CMAKE_COMMAND} -E  copy ${CMAKE_SOURCE_DIR}/editor/${loop} $<TARGET_FILE_DIR:krsyn_editor>/${loop})
endforeach(loop)

