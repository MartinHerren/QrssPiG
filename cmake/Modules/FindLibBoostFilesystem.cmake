# Try to find libboostprogramoptions
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibBoostFilesystem_INCLUDE_DIR NAMES boost/filesystem.hpp)

# Search first where lib is installed from source, then system one
find_library(LibBoostFilesystem_LIBRARY NAMES libboost_filesystem boost_filesystem PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibBoostFilesystem_LIBRARY NAMES libboost_filesystem boost_filesystem)

libfind_process(LibBoostFilesystem)
