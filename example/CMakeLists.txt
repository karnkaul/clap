project(clap-example)

set(TARGET_NAME clapp)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

add_executable(fib)
target_sources(fib PRIVATE fib.cpp)
target_link_libraries(fib PRIVATE clap::clap clap::compile-options)
add_test(NAME fib-test COMMAND fib "-s" "-a1" "-b2" "5")
