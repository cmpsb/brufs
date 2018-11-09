cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

# Settings start here

set(BRUFS_SANITIZE ON CACHE BOOL "Enable GCC sanitizers (only in debug builds)")
set(BRUFS_NANOSECOND_TIMESTAMP ON CACHE BOOL "If ON, uses clock_gettime(2); if OFF, uses time(2)")

# Settings end here

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

project(brufs LANGUAGES C CXX)

include(extract_git_info)
extract_git_info(WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
if (GIT_TAG_HAS_VERSION)
    set(PROJECT_VERSION_MAJOR "${GIT_TAG_MAJOR}")
    set(PROJECT_VERSION_MINOR "${GIT_TAG_MINOR}")
    set(PROJECT_VERSION_PATCH "${GIT_TAG_PATCH}")
    list(APPEND BRUFS_FLAGS git)

    if (GIT_DIRTY_TREE)
        list(APPEND BRUFS_FLAGS dirty)
    endif ()
else ()
    set(PROJECT_VERSION_MAJOR 0)
    set(PROJECT_VERSION_MINOR 0)
    set(PROJECT_VERSION_PATCH 0)
endif ()

STRING(TIMESTAMP BRUFS_BUILD_DATE UTC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -march=haswell -mtune=native")

if(NOT CMAKE_BUILD_TYPE)
    if ((NOT GIT_BRANCH) OR ("${GIT_BRANCH}" STREQUAL "master"))
        set(CMAKE_BUILD_TYPE "release")
    else ()
        set(CMAKE_BUILD_TYPE "debug")
    endif ()
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
list(APPEND BRUFS_FLAGS "${BUILD_TYPE}")
if ("${BUILD_TYPE}" STREQUAL "release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -flto -fuse-linker-plugin -DNDEBUG")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

    if (BRUFS_SANITIZE)
        list(APPEND BRUFS_FLAGS "sanitize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")
    endif ()
endif ()

set(BRUFS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(BRUFS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(BRUFS_VERSION_PATCH ${PROJECT_VERSION_PATCH})

add_subdirectory(libbrufs)
add_subdirectory(brufscli)
add_subdirectory(brufuse)
