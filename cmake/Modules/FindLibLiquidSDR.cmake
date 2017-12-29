# Try to find libliquid
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibLiquidSDR_INCLUDE_DIR NAMES liquid/liquid.h)

# Search first where lib is installed from source, then system one
find_library(LibLiquidSDR_LIBRARY NAMES libliquid liquid PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibLiquidSDR_LIBRARY NAMES libliquid liquid)

libfind_process(LibLiquidSDR)
