cmake_minimum_required(VERSION 3.18)

project(clap VERSION "0.3.0")

set(is_root_project OFF)

if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  set(is_root_project ON)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CLAP_BUILD_EXAMPLE "Build clap example" ${is_root_project})
option(CLAP_BUILD_TEST "Build clap tests" ${is_root_project})

add_library(${PROJECT_NAME}-compile-options INTERFACE)
add_library(${PROJECT_NAME}::compile-options ALIAS ${PROJECT_NAME}-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_library(clap)
add_library(clap::clap ALIAS clap)

target_compile_features(clap PUBLIC cxx_std_20)

target_include_directories(clap SYSTEM INTERFACE
  include
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_include_directories(clap PRIVATE
  include
  .
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${PROJECT_NAME}::compile-options
)

target_sources(clap PRIVATE
  include/clap/detail/from_string.hpp
  include/clap/clap.hpp

  src/clap.cpp
  src/parser.cpp
  src/parser.hpp
)

get_target_property(sources clap SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/clap/build_version.hpp")
source_group("include/clap" FILES "${CMAKE_CURRENT_BINARY_DIR}/include/clap/build_version.hpp")

if(CLAP_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

if(CLAP_BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
