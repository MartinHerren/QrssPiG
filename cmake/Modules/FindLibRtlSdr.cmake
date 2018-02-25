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

include(CheckCXXSourceRuns)

set(CMAKE_REQUIRED_INCLUDES ${LibRtlSdr_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${LibRtlSdr_LIBRARY})

CHECK_SYMBOL_EXISTS(rtlsdr_set_tuner_bandwidth rtl-sdr.h LibRtlSdr_HAS_rtlsdr_set_tuner_bandwidth)

CHECK_CXX_SOURCE_RUNS("
#include <rtl-sdr.h>

int main(int argc, char *argv[]) {
  rtlsdr_get_device_count();

  return 0;
}
" LibRtlSdr_HAS_rtlsdr_get_device_count)
