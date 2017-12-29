# Try to find libboostprogramoptions
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibBoostProgramOptions_INCLUDE_DIR NAMES boost/program_options.hpp)

# Search first where lib is installed from source, then system one
find_library(LibBoostProgramOptions_LIBRARY NAMES libboost_program_options boost_program_options PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibBoostProgramOptions_LIBRARY NAMES libboost_program_options boost_program_options)

libfind_process(LibBoostProgramOptions)
