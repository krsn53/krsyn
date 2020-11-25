project(krsyn_raygui_editor)

option(BUILD_RAYGUI_EXAMPLES OFF)
option(BUILD_EXAMPLES OFF)

add_subdirectory(raylib)
add_subdirectory(raygui/projects/CMake)

file(GLOB editor_src *.h *.c)
add_executable(krsyn_raygui_editor ${editor_src})
target_link_libraries(krsyn_raygui_editor raylib raygui krsyn)


