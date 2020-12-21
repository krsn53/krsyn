project(krsyn_tools)

option(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}/tools")

option(BUILD_RAYGUI_EXAMPLES OFF)
option(BUILD_GAMES OFF)
option(BUILD_EXAMPLES OFF)

file(GLOB editor_src editor/*.h editor/*.c)
add_executable(krsyn_editor ${editor_src})

if(NOT ${PLATFORM} STREQUAL Web)
    add_subdirectory(rtmidi)
    target_link_libraries(krsyn_editor rtmidi)
endif()

add_subdirectory(raylib)
add_subdirectory(raygui/projects/CMake)
target_link_libraries(krsyn_editor raylib raygui krsyn)

set_property(TARGET krsyn_editor PROPERTY C_STANDARD 11)

if(${PLATFORM} MATCHES "Web")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
  # Since WASM is used, ALLOW_MEMORY_GROWTH has no extra overheads
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/raylib/src/shell.html --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources")
  set(OUTPUT_EXT ".html")

  # Remove the -rdynamic flag because otherwise emscripten
  # does not generate HTML+JS+WASM files, only a non-working
  # and fat HTML
  string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS ${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS})

  set_target_properties(
       krsyn_editor
       PROPERTIES
           OUTPUT_NAME "krsyn_editor"
           SUFFIX ".html"
   )

else()
    add_resources(krsyn_editor resources)
endif()
