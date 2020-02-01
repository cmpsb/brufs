find_path(Uv_INCLUDE_DIR
        NAMES uv.h
)

find_library(Uv_LIBRARY
        NAMES uv
)

libfind_process(Uv)
