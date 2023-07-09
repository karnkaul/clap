cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(clap-example)

if(NOT TARGET clap)
  find_package(clap CONFIG REQUIRED)
endif()

add_executable(fib)

target_sources(fib PRIVATE
  fib.cpp
)

target_link_libraries(fib PRIVATE
  clap::clap
  clap::clap-compile-options
)
