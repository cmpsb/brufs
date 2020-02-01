find_package(Fuse3 REQUIRED)
find_package(Uv REQUIRED)

set(SOURCE_FILES
    src/main.cpp
    src/message-io.cpp
    src/Util.cpp
    src/client/client.cpp
    src/client/mount-request.cpp
    src/client/status-request.cpp
    src/client/stop-request.cpp
    src/server/service.cpp
    src/server/request.cpp
    src/server/mount-request.cpp
    src/server/brufs.cpp
    src/server/fuse-iface.cpp
    src/server/FdAbst.cpp
    src/slopt/opt.c
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src ${Uv_INCLUDE_DIRS} ${Fuse3_INCLUDE_DIRS})

add_definitions(-DFUSE_USE_VERSION=31)
add_executable(brufuse ${SOURCE_FILES})

target_link_libraries(brufuse brufs ${Uv_LIBRARIES} ${Fuse3_LIBRARIES})

install(TARGETS brufuse RUNTIME DESTINATION bin)

cotire(brufuse)
