project(krsyn_tests LANGUAGES C)

option(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}/tests")



add_executable(wav_write_test wav_write_test.c)
target_link_libraries(wav_write_test krsyn)

add_executable(midi_file_test midi_file_test.c)
target_link_libraries(midi_file_test krsyn)

add_executable(io_test io_test.c)
target_link_libraries(io_test krsyn)

add_resources(midi_file_test resources)
