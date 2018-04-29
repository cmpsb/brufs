set(SOURCE_FILES
    src/iohelp.cpp
    src/freeman.cpp
    src/dust.cpp
    src/status.cpp
)

set(TEST_FILES
    test/catch.cpp
    test/btree.cpp
    test/btree-extent-values.cpp
    test/btree-range.cpp
    test/btree-remove.cpp
)

include_directories(include)

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
