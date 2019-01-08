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
    test/InodeCrud.cpp
    test/EntityCreator.cpp
)

configure_file(cmake/config.hpp.in config.hpp)

include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

add_library(brufs ${SOURCE_FILES})
target_compile_options(brufs PRIVATE -fno-exceptions)
set_target_properties(brufs PROPERTIES PUBLIC_HEADER
    include/brufs.h
    include/btree.h
    include/rtstructures.h
    include/status.h
    include/structures.h
)
install(
    TARGETS brufs
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/brufs
)

enable_testing()

add_executable(test_all ${TEST_FILES})
target_link_libraries(test_all brufs catch)
add_test(test_all test_all)
