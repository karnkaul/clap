add_library(${package_name})
add_library(${package_name}::${package_name} ALIAS ${package_name})

target_compile_features(${package_name} PUBLIC cxx_std_23)

target_link_libraries(${package_name} PRIVATE
  $<$<BOOL:${MINGW}>:stdc++exp>
)

target_include_directories(${package_name} PRIVATE
  src
)

set(version_module src/build_version.cppm)
configure_file(src/build_version.cppm.in ${version_module} @ONLY)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/*.hpp")

target_sources(${package_name} PUBLIC FILE_SET CXX_MODULES
  BASE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"

  FILES
  public/clap.cppm
  private/detail.cppm
  "${CMAKE_CURRENT_BINARY_DIR}/${version_module}"
)

target_sources(${package_name} PRIVATE
  src/clap.cpp
  src/detail.cpp
)
