# Try to find libgd
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibGd_INCLUDE_DIR NAMES gd.h)

# Search first where lib is installed from source, then system one
find_library(LibGd_LIBRARY NAMES libgd gd PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibGd_LIBRARY NAMES libgd gd)

libfind_process(LibGd)
