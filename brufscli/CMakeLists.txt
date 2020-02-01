set(SOURCE_FILES
    src/Action.cpp
    src/AddRootAction.cpp
    src/BrufsOpener.cpp
    src/CheckAction.cpp
    src/CopyInAction.cpp
    src/CopyOutAction.cpp
    src/FdAbst.cpp
    src/InitAction.cpp
    src/LsAction.cpp
    src/PathValidator.cpp
    src/TouchAction.cpp
    src/Util.cpp
    src/VersionAction.cpp
    src/MkdirAction.cpp
)

set(TEST_FILES
    test/PathValidator.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/libbrufs/include src)

add_library(brufscli ${SOURCE_FILES})
target_link_libraries(brufscli brufs slog slopt)

add_executable(brufscli-exe src/main.cpp)
set_target_properties(brufscli-exe PROPERTIES OUTPUT_NAME brufs)

target_link_libraries(brufscli-exe brufscli)

install(TARGETS brufscli-exe RUNTIME DESTINATION bin)

add_executable(brufscli-test ${TEST_FILES})
target_link_libraries(brufscli-test brufscli catch)
add_test(NAME test-brufscli COMMAND brufscli-test)
add_coverage(brufscli)
cotire(brufscli)
