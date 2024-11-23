if(NOT TARGET ktest)
  include(FetchContent)

  FetchContent_Declare(ktest
    GIT_REPOSITORY https://github.com/karnkaul/ktest
    GIT_TAG v0.1.1
    GIT_SHALLOW TRUE
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ktest"
  )
  FetchContent_MakeAvailable(ktest)
endif()
