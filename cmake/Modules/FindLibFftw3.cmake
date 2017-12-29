# Try to find libfftw3
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibFftw3_INCLUDE_DIR NAMES fftw3.h)

# Search first where lib is installed from source, then system one
find_library(LibFftw3_LIBRARY NAMES libfftw3f fftw3f PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibFftw3_LIBRARY NAMES libfftw3f fftw3f)

libfind_process(LibFftw3)
