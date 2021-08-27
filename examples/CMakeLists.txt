project(clap-example)

set(TARGET_NAME clapp)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

add_executable(fib)
target_sources(fib PRIVATE fib.cpp)
target_link_libraries(fib PRIVATE clap::clap)
add_test(NAME fib-test COMMAND fib "-s" "-a1" "-b2" "5")

add_executable(clapp)
target_sources(clapp PRIVATE clapp.cpp)
target_link_libraries(clapp PRIVATE clap::clap)
add_test(NAME clapp-test COMMAND clapp "--verbose" "greet" "abc" "reverse" "-s" "def")
