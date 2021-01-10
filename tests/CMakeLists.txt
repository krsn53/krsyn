project(krsyn_tests LANGUAGES C)

option(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}/tests")

add_executable(wav_write_test wav_write_test.c)
target_link_libraries(wav_write_test krsyn)

