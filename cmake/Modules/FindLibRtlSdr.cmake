# Try to find librtlsdr
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibRtlSdr_INCLUDE_DIR NAMES rtl-sdr.h)

# Search first where lib is installed from source, then system one
find_library(LibRtlSdr_LIBRARY NAMES librtlsdr rtlsdr PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibRtlSdr_LIBRARY NAMES librtlsdr rtlsdr)

libfind_process(LibRtlSdr)
