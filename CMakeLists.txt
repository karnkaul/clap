cmake_minimum_required(VERSION 3.23)

project(clap VERSION "0.5.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

option(CLAP_BUILD_EXAMPLES "Build clap examples" ${PROJECT_IS_TOP_LEVEL})
option(CLAP_BUILD_TESTS "Build clap tests" ${PROJECT_IS_TOP_LEVEL})
option(CLAP_INSTALL "Setup CMake install for ${PROJECT_NAME}" ${PROJECT_IS_TOP_LEVEL})

add_library(${PROJECT_NAME}-compile-options INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-compile-options ALIAS ${PROJECT_NAME}-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_subdirectory(lib)

if(CLAP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(CLAP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(ext)

  add_subdirectory(tests)
endif()
