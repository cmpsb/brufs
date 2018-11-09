function(extract_git_info)
    set(SINGLE_VALUE_PARAMS WORKING_DIRECTORY)
    cmake_parse_arguments(ARG  "" "${SINGLE_VALUE_PARAMS}" "" ${ARGN})

    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE GIT_DESCRIBE_STATUS
        OUTPUT_VARIABLE GIT_RAW_TAG
    )

    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE GIT_REV_PARSE_STATUS
        OUTPUT_VARIABLE GIT_RAW_COMMIT_HASH
    )

    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE GIT_BRANCH_STATUS
        OUTPUT_VARIABLE GIT_RAW_BRANCH
    )

    if ((${GIT_DESCRIBE_STATUS} EQUAL 0))
        string(REGEX REPLACE "\n$" "" GIT_TAG "${GIT_RAW_TAG}")
        set(GIT_TAG "${GIT_TAG}" PARENT_SCOPE)
        if ("${GIT_RAW_TAG}" MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+).*")
            set(GIT_TAG_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
            set(GIT_TAG_MINOR "${CMAKE_MATCH_2}" PARENT_SCOPE)
            set(GIT_TAG_PATCH "${CMAKE_MATCH_3}" PARENT_SCOPE)

            set(GIT_TAG_HAS_VERSION ON PARENT_SCOPE)
        else ()
            message(WARNING "Unable to extract a version from git tag ${GIT_RAW_TAG}")
        endif ()
    else ()
        message(WARNING "Unable to extract the latest git tag")
    endif ()

    if (${GIT_REV_PARSE_STATUS} EQUAL 0)
        string(STRIP "${GIT_RAW_COMMIT_HASH}" GIT_COMMIT)
        set(GIT_COMMIT "${GIT_COMMIT}" PARENT_SCOPE)
    else ()
        message(WARNING "Unable to extract the latest git commit hash")
    endif ()

    if (${GIT_BRANCH_STATUS} EQUAL 0)
        string(STRIP "${GIT_RAW_BRANCH}" GIT_BRANCH)
        set(GIT_BRANCH "${GIT_BRANCH}" PARENT_SCOPE)
    else ()
        message(WARNING "Unable to extract the current git branch")
    endif ()
endfunction()
