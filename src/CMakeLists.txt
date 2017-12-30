# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_definitions(-std=c++11)

# Look for mandatory libraries
find_package(LibBoostProgramOptions REQUIRED)
if (LibBoostProgramOptions_FOUND)
	include_directories(${LibBoostProgramOptions_INCLUDE_DIRS})
endif (LibBoostProgramOptions_FOUND)

find_package(LibYaml REQUIRED)
if (LibYaml_FOUND)
	include_directories(${LibYaml_INCLUDE_DIRS})
endif (LibYaml_FOUND)

find_package(LibFftw3 REQUIRED)
if (LibFftw3_FOUND)
	include_directories(${LibFftw3_INCLUDE_DIRS})
endif (LibFftw3_FOUND)

find_package(LibGd REQUIRED)
if (LibGd_FOUND)
	include_directories(${LibGd_INCLUDE_DIRS})
endif (LibGd_FOUND)

# Look for optional input module libraries
find_package(LibAlsa)
if (LibAlsa_FOUND)
	include_directories(${LibAlsa_INCLUDE_DIRS})
endif (LibAlsa_FOUND)

find_package(LibHackRF)
if (LibHackRF_FOUND)
	include_directories(${LibHackRF_INCLUDE_DIRS})
endif (LibHackRF_FOUND)

find_package(LibRtlSdr)
if (LibRtlSdr_FOUND)
	include_directories(${LibRtlSdr_INCLUDE_DIRS})
endif (LibRtlSdr_FOUND)

# Look for optional processing libraries
find_package(LibLiquidSDR)
if (LibLiquidSDR_FOUND)
	include_directories(${LibLiquidSDR_INCLUDE_DIR})
endif (LibLiquidSDR_FOUND)

# Don't look for librtfilter if libliquid available
if (NOT LibLiquidSDR_FOUND)
	find_package(LibRTFilter)
	if (LibRTFilter_FOUND)
		include_directories(${LibRTFilter_INCLUDE_DIR})
	endif (LibRTFilter_FOUND)
endif (NOT LibLiquidSDR_FOUND)

# Loop for optional upload libraries
find_package(LibSSH)
if (LibSSH_FOUND)
	include_directories(${LibSSH_INCLUDE_DIR})
endif (LibSSH_FOUND)

find_package(LibCurl)
if (LibCurl_FOUND)
	include_directories(${LibCurl_INCLUDE_DIR})
endif (LibCurl_FOUND)

list(APPEND QRSSPIG_SOURCES main.cpp QrssPiG.cpp QGInputDevice.cpp QGInputStdIn.cpp QGProcessor.cpp QGImage.cpp QGUploader.cpp QGUploaderLocal.cpp)

if (LibAlsa_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputAlsa.cpp)
endif (LibAlsa_FOUND)

if (LibHackRF_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputHackRF.cpp)
endif (LibHackRF_FOUND)

if (LibRtlSdr_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputRtlSdr.cpp)
endif (LibRtlSdr_FOUND)

if (LibSSH_FOUND)
	list(APPEND QRSSPIG_SOURCES QGUploaderSCP.cpp)
endif (LibSSH_FOUND)

if (LibCurl_FOUND)
	list(APPEND QRSSPIG_SOURCES QGUploaderFTP.cpp)
endif (LibCurl_FOUND)

add_executable (qrsspig ${QRSSPIG_SOURCES})

target_link_libraries(qrsspig pthread)

if (LibBoostProgramOptions_FOUND)
	target_link_libraries(qrsspig ${LibBoostProgramOptions_LIBRARIES})
endif (LibBoostProgramOptions_FOUND)

if (LibYaml_FOUND)
	target_link_libraries(qrsspig ${LibYaml_LIBRARIES})
endif (LibYaml_FOUND)

if (LibFftw3_FOUND)
	target_link_libraries(qrsspig ${LibFftw3_LIBRARIES})
endif (LibFftw3_FOUND)

if (LibGd_FOUND)
	target_link_libraries(qrsspig ${LibGd_LIBRARIES})
endif (LibGd_FOUND)

if (LibAlsa_FOUND)
	target_link_libraries(qrsspig ${LibAlsa_LIBRARIES})
	set(HAVE_LIBALSA 1)
endif (LibAlsa_FOUND)

if (LibHackRF_FOUND)
	target_link_libraries(qrsspig ${LibHackRF_LIBRARIES})
	set(HAVE_LIBHACKRF 1)
endif (LibHackRF_FOUND)

if (LibRtlSdr_FOUND)
	target_link_libraries(qrsspig ${LibRtlSdr_LIBRARIES})
	set(HAVE_LIBRTLSDR 1)
endif (LibRtlSdr_FOUND)

if (LibLiquidSDR_FOUND)
	target_link_libraries(qrsspig ${LibLiquidSDR_LIBRARIES})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LibLiquidSDR_FOUND)

if (LibRTFilter_FOUND)
	target_link_libraries(qrsspig ${LibRTFilter_LIBRARIES})
	set(HAVE_LIBRTFILTER 1)
endif (LibRTFilter_FOUND)

if (LibSSH_FOUND)
	target_link_libraries(qrsspig ${LibSSH_LIBRARIES})
	set(HAVE_LIBSSH 1)
endif (LibSSH_FOUND)

if (LibCurl_FOUND)
	target_link_libraries(qrsspig ${LibCurl_LIBRARIES})
	set(HAVE_LIBCURL 1)
endif (LibCurl_FOUND)

# Generate and include Config.h
configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")
include_directories("${PROJECT_BINARY_DIR}")

install(TARGETS qrsspig RUNTIME DESTINATION bin)
