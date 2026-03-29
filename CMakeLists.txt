cmake_minimum_required(VERSION 3.24)

set(package_name clap)
project(${package_name} VERSION "0.1.1")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

option(CLAP_BUILD_EXAMPLE "Build clap example" ${PROJECT_IS_TOP_LEVEL})
option(CLAP_BUILD_TESTS "Build clap tests" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(lib)

if(CLAP_BUILD_EXAMPLE)
  # add_subdirectory(example)
endif()

if(CLAP_BUILD_TESTS)
  if(NOT TARGET klib::klib-test-main)
    include(FetchContent)
    FetchContent_Declare(klib
      GIT_REPOSITORY https://github.com/karnkaul/klib
      GIT_TAG v0.6.1
      SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/src/klib"
    )
    set(KLIB_UNIT_TEST ON)
    FetchContent_MakeAvailable(klib)
  endif()

  enable_testing()
  add_subdirectory(tests)
endif()
