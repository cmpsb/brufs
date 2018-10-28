set(SOURCE_FILES
    src/main.cpp
    src/message-io.cpp
    src/client/client.cpp
    src/client/mount-request.cpp
    src/client/status-request.cpp
    src/client/stop-request.cpp
    src/server/service.cpp
    src/server/request.cpp
    src/slopt/opt.c
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src)

add_executable(brufuse ${SOURCE_FILES})

target_link_libraries(brufuse brufs uv)
