cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

project(brufs LANGUAGES C CXX VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-g -Og -Wall -Wextra -pedantic -flto -fuse-linker-plugin")

set(BRUFS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(BRUFS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(BRUFS_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(BRUFS_NANOSECOND_TIMESTAMP ON CACHE BOOL "If ON, uses clock_gettime(2); if OFF, uses time(2)")

add_subdirectory(libbrufs)
add_subdirectory(brufscli)
add_subdirectory(brufuse)
