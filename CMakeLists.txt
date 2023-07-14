cmake_minimum_required(VERSION 3.23)

project(clap VERSION "0.4.3")

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
add_library(${PROJECT_NAME}::${PROJECT_NAME}-compile-options ALIAS ${PROJECT_NAME}-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_library(clap)
add_library(clap::clap ALIAS clap)

target_compile_features(clap PUBLIC cxx_std_20)

target_include_directories(clap PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)

target_include_directories(clap PRIVATE
  .
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${PROJECT_NAME}::${PROJECT_NAME}-compile-options
)

target_sources(clap PRIVATE
  src/clap.cpp
  src/parser.cpp
  src/parser.hpp
)

target_sources(clap PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  include/clap/detail/from_string.hpp
  include/clap/clap.hpp
)

get_target_property(sources ${PROJECT_NAME} SOURCES)
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

# install
option(CLAP_INSTALL "Setup CMake install for clap" ${is_root_project})

if(CLAP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(TARGETS
    ${PROJECT_NAME}
    ${PROJECT_NAME}-compile-options

    EXPORT ${PROJECT_NAME}-targets

    FILE_SET HEADERS
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp"

    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  )

  install(EXPORT
    ${PROJECT_NAME}-targets

    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION share/cmake/${PROJECT_NAME}
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION share/cmake/${PROJECT_NAME}
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"

    DESTINATION share/cmake/${PROJECT_NAME}
  )
endif()
