cmake_minimum_required(VERSION 3.20)
project(load_balancer_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Conan injects Boost. CMake expects package name 'Boost'
# and uses the imported target 'Boost::boost'.
find_package(Boost REQUIRED)

# System threads
find_package(Threads REQUIRED)

add_executable(rate_limiter src/rate_limiter.cpp)

# Link Boost + Threads
target_link_libraries(rate_limiter PRIVATE Boost::boost Threads::Threads)
