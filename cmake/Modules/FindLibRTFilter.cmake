# Try to find librtfilter
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibRTFilter_INCLUDE_DIR NAMES rtfilter.h)

# Search first where lib is installed from source, then system one
find_library(LibRTFilter_LIBRARY NAMES librtfilter rtfilter PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibRTFilter_LIBRARY NAMES librtfilter rtfilter)

libfind_process(LibRTFilter)
