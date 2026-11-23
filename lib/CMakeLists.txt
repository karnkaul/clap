add_library(${package_name})
add_library(${package_name}::${package_name} ALIAS ${package_name})

target_compile_features(${package_name} PUBLIC cxx_std_23)

target_link_libraries(${package_name} PRIVATE
  $<$<BOOL:${MINGW}>:stdc++exp>
)

set(version_module public/build_version.cppm)
configure_file(${version_module}.in ${version_module} @ONLY)

target_sources(${package_name} PUBLIC FILE_SET CXX_MODULES
  BASE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"

  FILES
  public/types.cppm
  public/scanner.cppm
  public/spec.cppm
  detail/detail.cppm
  public/parser.cppm
  public/clap.cppm
  "${CMAKE_CURRENT_BINARY_DIR}/${version_module}"
)

target_sources(${package_name} PRIVATE
  src/types.cpp
  src/scanner.cpp
  src/spec.cpp
  src/detail.cpp
  src/parser.cpp
  src/clap.cpp
)
