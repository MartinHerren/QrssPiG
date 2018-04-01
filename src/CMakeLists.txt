# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_definitions(-std=c++11)

# Look for mandatory libraries
find_package(LibBoostProgramOptions REQUIRED)
if (LibBoostProgramOptions_FOUND)
	include_directories(${LibBoostProgramOptions_INCLUDE_DIRS})
endif (LibBoostProgramOptions_FOUND)

find_package(LibBoostFilesystem REQUIRED)
if (LibBoostFilesystem_FOUND)
	include_directories(${LibBoostFilesystem_INCLUDE_DIRS})
endif (LibBoostFilesystem_FOUND)

find_package(LibBoostSystem REQUIRED)
if (LibBoostSystem_FOUND)
	#include_directories(${LibBoostSystem_INCLUDE_DIRS})
endif (LibBoostSystem_FOUND)

find_package(LibYaml REQUIRED)
if (LibYaml_FOUND)
	include_directories(${LibYaml_INCLUDE_DIRS})
endif (LibYaml_FOUND)

find_package(LibFftw3 REQUIRED)
if (LibFftw3_FOUND)
	include_directories(${LibFftw3_INCLUDE_DIRS})
endif (LibFftw3_FOUND)

# Look for optional processing libraries
find_package(LibLiquidSDR)
if (LibLiquidSDR_FOUND)
	include_directories(${LibLiquidSDR_INCLUDE_DIRS})
endif (LibLiquidSDR_FOUND)

# Don't look for librtfilter if libliquid available
if (NOT LibLiquidSDR_FOUND)
	find_package(LibRTFilter)
	if (LibRTFilter_FOUND)
		include_directories(${LibRTFilter_INCLUDE_DIRS})
	endif (LibRTFilter_FOUND)
endif (NOT LibLiquidSDR_FOUND)

list(APPEND QRSSPIG_SOURCES main.cpp QrssPiG.cpp QGUtils.cpp QGInputDevice.cpp QGProcessor.cpp QGOutput.cpp QGUploader.cpp)

add_executable (qrsspig ${QRSSPIG_SOURCES})

target_link_libraries(qrsspig pthread)

# Dynamic linking library
target_link_libraries(qrsspig dl)

if (LibBoostProgramOptions_FOUND)
	target_link_libraries(qrsspig ${LibBoostProgramOptions_LIBRARIES})
endif (LibBoostProgramOptions_FOUND)

if (LibBoostFilesystem_FOUND)
	target_link_libraries(qrsspig ${LibBoostFilesystem_LIBRARIES})
endif (LibBoostFilesystem_FOUND)

if (LibBoostSystem_FOUND)
	target_link_libraries(qrsspig ${LibBoostSystem_LIBRARIES})
endif (LibBoostSystem_FOUND)

if (LibYaml_FOUND)
	target_link_libraries(qrsspig ${LibYaml_LIBRARIES})
endif (LibYaml_FOUND)

if (LibFftw3_FOUND)
	target_link_libraries(qrsspig ${LibFftw3_LIBRARIES})
endif (LibFftw3_FOUND)

if (LibLiquidSDR_FOUND)
	target_link_libraries(qrsspig ${LibLiquidSDR_LIBRARIES})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LibLiquidSDR_FOUND)

if (LibRTFilter_FOUND)
	target_link_libraries(qrsspig ${LibRTFilter_LIBRARIES})
	set(HAVE_LIBRTFILTER 1)
endif (LibRTFilter_FOUND)

include_directories("${PROJECT_SOURCE_DIR}/src")

# Generate and include Config.h
configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")
include_directories("${PROJECT_BINARY_DIR}")

set_target_properties(qrsspig PROPERTIES PUBLIC_HEADER "QGUploader.h;QGOutput.h;QGInputDevice.h")

install(TARGETS qrsspig RUNTIME DESTINATION bin
	PUBLIC_HEADER DESTINATION include
)

add_subdirectory(plugins)
