# Try to find libhackrf
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibHackRF_INCLUDE_DIR NAMES libhackrf/hackrf.h)

# Search first where lib is installed from source, then system one
find_library(LibHackRF_LIBRARY NAMES libhackrf hackrf PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibHackRF_LIBRARY NAMES libhackrf hackrf)

libfind_process(LibHackRF)
