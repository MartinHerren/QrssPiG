# Try to find libasound2
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibAlsa_INCLUDE_DIR NAMES alsa/asoundlib.h)

# Search first where lib is installed from source, then system one
find_library(LibAlsa_LIBRARY NAMES libasound asound PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibAlsa_LIBRARY NAMES libasound asound)

libfind_process(LibAlsa)
