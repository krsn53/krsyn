project(krsyn_editor)

option(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}/old_editor")

function(target_link as lib proj)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(${as} REQUIRED ${lib})
    target_include_directories(${proj} PUBLIC ${${as}_INCLUDE_DIRS})
    target_link_directories(${proj} PUBLIC ${${as}_LIBRARY_DIRS})
    target_compile_options(${proj} PUBLIC ${${as}_CFLAGS_OTHER})
    target_link_libraries(${proj} ${${as}_LIBRARIES})
endfunction()

file(GLOB krsyn_editor_src *.h *.c)
add_executable(krsyn_editor ${krsyn_editor_src})
target_link(AL openal krsyn_editor)
target_link(GTK gtk+-3.0 krsyn_editor)
target_link_libraries(krsyn_editor krsyn)
add_resources(krsyn_editor resources)
