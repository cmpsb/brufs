cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

project(brufs LANGUAGES C CXX VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

set(BRUFS_SANITIZE ON CACHE BOOL "Enable GCC sanitizers (only in debug builds)")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -march=haswell -mtune=native")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -flto -fuse-linker-plugin -DNDEBUG")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    if (BRUFS_SANITIZE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")
    endif ()
endif ()

set(BRUFS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(BRUFS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(BRUFS_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(BRUFS_NANOSECOND_TIMESTAMP ON CACHE BOOL "If ON, uses clock_gettime(2); if OFF, uses time(2)")

add_subdirectory(libbrufs)
add_subdirectory(brufscli)
add_subdirectory(brufuse)
