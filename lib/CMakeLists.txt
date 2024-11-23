
add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${PROJECT_NAME}::${PROJECT_NAME}-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  .
  src
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  include/clap/app_info.hpp
  include/clap/arg.hpp
  include/clap/binding.hpp
  include/clap/concepts.hpp
  include/clap/parse.hpp
  include/clap/result.hpp
)

target_sources(${PROJECT_NAME} PRIVATE
  src/arg.cpp
  src/parse.cpp
  src/parser.hpp
  src/scanner.hpp
  src/token.hpp
)

get_target_property(sources ${PROJECT_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")
source_group("include/${PROJECT_NAME}" FILES "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp"
)

if(CLIQER_INSTALL)
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
