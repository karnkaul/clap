cmake_minimum_required(VERSION 3.14)

set(CLAP_VERSION "0.2" CACHE STRING "clap version" FORCE)
project(clap VERSION ${CLAP_VERSION})

option(CLAP_BUILD_EXAMPLES "Build clap example" OFF)

# TODO: Remove
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")

add_library(clap)
add_library(clap::clap ALIAS clap)
target_compile_features(clap PUBLIC cxx_std_17)
target_include_directories(clap INTERFACE SYSTEM include)
target_include_directories(clap PRIVATE include src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_sources(clap PRIVATE
	include/clap/clap_types.hpp
	include/clap/clap.hpp
	src/clap.cpp
)
get_target_property(sources clap SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
configure_file(src/clap_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/clap_version.hpp")

if(UNIX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	target_compile_options(clap PRIVATE -Wall -Wextra $<$<NOT:$<CONFIG:Debug>>:-Werror>)
endif()

if(CLAP_BUILD_EXAMPLES)
	enable_testing()
	add_subdirectory(examples)
endif()
