cmake_minimum_required(VERSION 3.3)

project(clap)

file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${INCLUDES})

add_library(clap INTERFACE ${INCLUDES})
add_library(clap::clap ALIAS clap)
target_compile_features(clap INTERFACE cxx_std_17)
target_include_directories(clap INTERFACE include)
