cmake_minimum_required(VERSION 3.5)

project(krsyn LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()

file(GLOB krsyn_src *.c *.h)
add_library(krsyn ${krsyn_src})
target_include_directories(krsyn PUBLIC  ./)

add_executable(wav_write_test tests/wav_write_test.c)
target_link_libraries(wav_write_test krsyn)

# basic files

add_subdirectory(thirdparty/wxWidgets)
