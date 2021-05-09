project(clap-example)

set(TARGET_NAME clapp)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES clapp.cpp)
add_executable(${TARGET_NAME} clapp.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE clap::clap)
