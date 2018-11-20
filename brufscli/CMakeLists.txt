set(SOURCE_FILES
    src/init.cpp
    src/check.cpp
    src/add-root.cpp
    src/FdAbst.cpp
    src/prompt.cpp
    src/ls.cpp
    src/mkdir.cpp
    src/Util.cpp
    src/touch.cpp
    src/copy_in.cpp
    src/copy_out.cpp
    src/version.cpp
    src/PathValidator.cpp
)

set(TEST_FILES
    test/PathValidator.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src)

add_executable(brufscli src/main.cpp ${SOURCE_FILES})
set_target_properties(brufscli PROPERTIES OUTPUT_NAME brufs)

target_link_libraries(brufscli brufs)

enable_testing()

add_executable(test ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(test brufs catch)
add_test(test test)
