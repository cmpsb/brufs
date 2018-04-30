set(SOURCE_FILES
    src/main.cpp
    src/init.cpp
    src/check.cpp
    src/add-root.cpp
    src/fd_abst.cpp
    src/prompt.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src)

add_executable(brufscli ${SOURCE_FILES})
set_target_properties(brufscli PROPERTIES OUTPUT_NAME brufs)

target_link_libraries(brufscli brufs)
