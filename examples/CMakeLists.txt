project(clap-examples)

function(add_example name)
  add_executable(${name})
  target_link_libraries(${name} PRIVATE clap::clap)
  target_sources(${name} PRIVATE ${name}.cpp)
endfunction(add_example)

add_example("square")
add_example("execute")
