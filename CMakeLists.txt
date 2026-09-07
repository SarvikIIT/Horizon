cmake_minimum_required(VERSION 3.20)
project(horizon CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(horizon
  src/offline.cpp
  src/colourer.cpp
  src/first_fit.cpp
  src/kierstead_trotter.cpp
  src/lookahead.cpp
  src/generators.cpp
)
target_include_directories(horizon PUBLIC src)

add_executable(bench bench/main.cpp)
target_link_libraries(bench PRIVATE horizon)

# Exhaustive game solver. Standalone: it works on the abstract game, not on the
# Interval/Instance types, so it links nothing.
add_executable(solve bench/solve.cpp)

option(HORIZON_BUILD_TESTS "Build the GoogleTest suite" ON)
if(HORIZON_BUILD_TESTS)
  enable_testing()
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(horizon_tests
    tests/test_offline.cpp
    tests/test_colourers.cpp
    tests/test_generators.cpp
  )
  target_link_libraries(horizon_tests PRIVATE horizon GTest::gtest GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(horizon_tests)
endif()
