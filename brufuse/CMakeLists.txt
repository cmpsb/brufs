set(SOURCE_FILES
    src/main.cpp
    src/slopt/opt.c
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src)

add_executable(brufuse ${SOURCE_FILES})

target_link_libraries(brufuse brufs)
