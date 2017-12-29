# Try to find libcurl
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibCurl_INCLUDE_DIR NAMES curl/curl.h)

# Search first where lib is installed from source, then system one
find_library(LibCurl_LIBRARY NAMES libcurl curl PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibCurl_LIBRARY NAMES libcurl curl)

libfind_process(LibCurl)
