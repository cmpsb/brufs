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
)

set(TEST_FILES
    test/catch.cpp
    test/btree.cpp
    test/btree-extent-values.cpp
    test/btree-range.cpp
    test/btree-remove.cpp
    test/btree-update.cpp
)

configure_file(cmake/version.in.hpp version-info.hpp)

include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

add_library(brufs ${SOURCE_FILES})
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
target_link_libraries(test_all brufs)
add_test(test_all test_all)
