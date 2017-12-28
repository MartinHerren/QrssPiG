# Try to find libyaml
#
# Once successfully done this will define
#  LIB

include(LibFindMacros)

find_path(LibYaml_INCLUDE_DIR NAMES yaml-cpp/yaml.h)

# Search first where lib is installed from source, then system one
find_library(LibYaml_LIBRARY NAMES libyaml-cpp yaml-cpp PATHS /usr/local/lib /usr/lib NO_DEFAULT_PATH)
find_library(LibYaml_LIBRARY NAMES libyaml-cpp yaml-cpp)

libfind_process(LibYaml)
