# Try to find libliquid
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LIBLIQUIDSDR_INCLUDE_DIR NAMES liquid/liquid.h)
find_library(LIBLIQUIDSDR_LIBRARY NAMES libliquid liquid)

libfind_process(LIBLIQUIDSDR)
