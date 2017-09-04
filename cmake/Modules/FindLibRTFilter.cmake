# Try to find librtfilter
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LIBRTFILTER_INCLUDE_DIR NAMES rtfilter.h)
find_library(LIBRTFILTER_LIBRARY NAMES librtfilter rtfilter)

#set(LIBRTFILTER_PROCESS_INCLUDES LIBRTFILTER_INCLUDE_DIR LIBRTFILTER_INCLUDE_DIRS)
#set(LIBRTFILTER_PROCESS_LIBS LIBRTFILTER_LIBRARY LIBRTFILTER_LIBRARIES)
libfind_process(LIBRTFILTER)
