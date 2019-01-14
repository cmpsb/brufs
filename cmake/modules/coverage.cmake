function(enable_coverage)
    cmake_parse_arguments(COVERAGE "" "OUTPUT_DIRECTORY" "" ${ARGN})

    if (NOT COVERAGE_OUTPUT_DIRECTORY)
        set(COVERAGE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)
        set(COVERAGE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/coverage PARENT_SCOPE)
    endif ()
    file(MAKE_DIRECTORY "${COVERAGE_OUTPUT_DIRECTORY}")
    add_custom_target(coverage)
endfunction()

function(add_coverage)
    set(OPTIONS NO_OUTPUT)
    set(OV_KEYWORDS SOURCE_DIR TARGET_DIR)
    set(MV_KEYWORDS EXCLUDE FILTER DEPENDS)
    cmake_parse_arguments(COV "${OPTIONS}" "${OV_KEYWORDS}" "${MV_KEYWORDS}" ${ARGN})

    list(GET COV_UNPARSED_ARGUMENTS 0 RAW_TARGET)
    set(TARGET "${RAW_TARGET}-coverage")

    if (NOT COV_TARGET_DIR)
        set(COV_TARGET_DIR "${COVERAGE_OUTPUT_DIRECTORY}/${RAW_TARGET}")
    endif ()
    file(MAKE_DIRECTORY "${COV_TARGET_DIR}")

    if (NOT COV_SOURCE_DIR)
        set(COV_SOURCE_DIR "${CMAKE_SOURCE_DIR}/${RAW_TARGET}.*")
    endif ()

    set(GCOVR_COMMAND_LINE gcovr -sr "${CMAKE_SOURCE_DIR}")

    foreach (E ${COV_EXCLUDE})
        list(APPEND GCOVR_COMMAND_LINE --exclude "${E}")
    endforeach ()

    list(APPEND GCOVR_COMMAND_LINE --filter "${COV_SOURCE_DIR}")

    foreach (F ${COV_FILTER})
        list(APPEND GCOVR_COMMAND_LINE --filter "${F}")
    endforeach ()

    if (NOT COV_NO_OUTPUT)
        list(APPEND GCOVR_COMMAND_LINE --html-details -o "${COV_TARGET_DIR}/coverage")
    endif ()

    add_custom_target(${TARGET} COMMAND ${GCOVR_COMMAND_LINE} DEPENDS ${COV_DEPENDS})
    add_dependencies(coverage ${TARGET})
endfunction()
