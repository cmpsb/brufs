cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

project(brufs LANGUAGES CXX VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -pedantic")

add_subdirectory(libbrufs)
