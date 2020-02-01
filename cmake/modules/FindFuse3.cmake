include(LibFindMacros)

libfind_pkg_check_modules(Fuse3_PKGCONF fuse3)
find_path(Fuse3_INCLUDE_DIR
        NAMES fuse.h
        PATHS ${Fuse3_PKGCONF_INCLUDE_DIRS}
        PATH_SUFFIXES fuse3
)

find_library(Fuse3_LIBRARY
        NAMES fuse3
        PATHS ${Fuse3_PKCONF_LIBRARY_DIRS}
)
libfind_process(Fuse3)