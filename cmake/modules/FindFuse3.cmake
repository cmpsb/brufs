include(LibFindMacros)

libfind_pkg_check_modules(Fuse3_PKGCONF fuse3)
find_path(Fuse3_INCLUDE_DIR
        NAMES fuse.h
        HINTS ${Fuse3_PKGCONF_INCLUDE_DIRS}
        PATH_SUFFIXES fuse3
)

find_library(Fuse3_LIBRARY
        NAMES fuse3 fuse3.so.3 libfuse3.so.3
        HINTS ${Fuse3_PKGCONF_LIBRARY_DIRS}
        REQUIRED
)
libfind_process(Fuse3)
