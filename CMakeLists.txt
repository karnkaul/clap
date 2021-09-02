cmake_minimum_required(VERSION 3.14)
enable_language(CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_DEBUG_POSTFIX "-d")
set(CLAP_VERSION "0.2" CACHE STRING "clap version" FORCE)
project(clap VERSION ${CLAP_VERSION})

option(CLAP_BUILD_EXAMPLES "Build clap example" OFF)

add_library(clap)
add_library(clap::clap ALIAS clap)
target_compile_features(clap PUBLIC cxx_std_17)
target_include_directories(clap SYSTEM INTERFACE 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(clap PRIVATE include src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_sources(clap PRIVATE
	include/clap/clap.hpp
	include/clap/clap_types.hpp
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

# install and export targets
install(TARGETS clap EXPORT clap-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# install exported targets
install(EXPORT clap-targets
	FILE clap-targets.cmake
	NAMESPACE clap::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap
)
# install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
# configure clap-config.cmake
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/clap-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap
)
# install clap-config.cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/clap-config.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap
)
