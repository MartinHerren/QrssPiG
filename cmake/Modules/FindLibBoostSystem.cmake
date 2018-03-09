# Try to find libboostprogramoptions
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

# Search first where lib is installed from source, then system one
find_library(LibBoostSystem_LIBRARY NAMES libboost_system boost_system PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibBoostSystem_LIBRARY NAMES libboost_system boost_system)

libfind_process(LibBoostSystem)
