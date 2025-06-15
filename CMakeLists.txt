cmake_minimum_required(VERSION 3.20)
project(load_balancer_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Conan injects Boost
find_package(Boost REQUIRED)

find_package(nlohmann_json REQUIRED)

find_package(GTest REQUIRED)

# System threads
find_package(Threads REQUIRED)

add_executable(rate_limiter src/rate_limiter.cpp)

include_directories(rate_limiter PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link Boost + Threads
target_link_libraries(rate_limiter PRIVATE Boost::boost Threads::Threads)
target_link_libraries(rate_limiter PRIVATE nlohmann_json::nlohmann_json)

enable_testing()

add_executable(unit_tests
  tests/token_bucket_test.cpp
  tests/register_handler_test.cpp)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(unit_tests PRIVATE
  Boost::boost
  Threads::Threads
  GTest::gtest_main
  nlohmann_json::nlohmann_json)
add_test(NAME unit_tests COMMAND unit_tests)
