# Try to find libssh
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LIBSSH_INCLUDE_DIR NAMES libssh/libssh.h)

# Search first where lib is installed from source, then system one
find_library(LIBSSH_LIBRARY NAMES libssh ssh PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LIBSSH_LIBRARY NAMES libssh ssh)

libfind_process(LIBSSH)
