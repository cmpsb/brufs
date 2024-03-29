set(SOURCE_FILES
    src/Brufs.cpp
    src/AbstIO.cpp
    src/Header.cpp
    src/iohelp.cpp
    src/Root.cpp
    src/status.cpp
    src/Version.cpp
    src/xxhash/xxhash.c
    src/File.cpp
    src/Directory.cpp
    src/Inode.cpp
    src/Timestamp.cpp
    src/BuildInfo.cpp
    src/PathParser.cpp
    src/EntityCreator.cpp
    src/InodeIdGenerator.cpp
    src/InodeHeaderBuilder.cpp
    src/PrettyPrint.cpp
    src/RootHeader.cpp
)

set(TEST_FILES
    test/btree.cpp
    test/btree-extent-values.cpp
    test/btree-range.cpp
    test/btree-remove.cpp
    test/btree-update.cpp
    test/String.cpp
    test/BuildInfo.cpp
    test/PathParser.cpp
    test/Path.cpp
    test/BrufsInit.cpp
    test/Root.cpp
    test/PrettyPrint.cpp
    test/InodeCrud.cpp
    test/EntityCreator.cpp
    test/File.cpp
)

configure_file(cmake/config.hpp.in config.hpp)

include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

add_library(brufs ${SOURCE_FILES})
target_compile_options(brufs PRIVATE -fno-exceptions)
install(
    TARGETS brufs
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/brufs)

if (BRUFS_TESTS)
    add_executable(libbrufs-test ${TEST_FILES})
    target_link_libraries(libbrufs-test brufs catch)
    add_test(NAME test-libbrufs COMMAND libbrufs-test)
    add_coverage(libbrufs)
endif ()

cotire(brufs)
