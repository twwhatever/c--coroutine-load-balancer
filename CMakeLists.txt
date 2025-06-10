cmake_minimum_required(VERSION 3.20)
project(load_balancer_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Conan injects the Boost package, which CMake expects to be named
# "Boost". The generated CMake targets use the lowercase
# namespace, e.g. "boost::boost".
find_package(Boost REQUIRED)

# System threads
find_package(Threads REQUIRED)

add_executable(rate_limiter src/rate_limiter.cpp)

# Link Boost + Threads
target_link_libraries(rate_limiter PRIVATE boost::boost Threads::Threads)
