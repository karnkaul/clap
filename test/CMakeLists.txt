project(clap-test)

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE clap::clap clap::compile-options)
target_sources(${PROJECT_NAME} PRIVATE test.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ..)

add_test(NAME clap-test COMMAND ${PROJECT_NAME})
