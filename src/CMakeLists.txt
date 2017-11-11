# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_definitions(-std=c++11)

# Look for optional packages
find_package(LibAlsa)
if (LIBALSA_FOUND)
	include_directories(${LIBALSA_INCLUDE_DIRS})
endif (LIBALSA_FOUND)

find_package(LibHackRF)
if (LIBHACKRF_FOUND)
	include_directories(${LIBHACKRF_INCLUDE_DIRS})
endif (LIBHACKRF_FOUND)

find_package(LibRtlSdr)
if (LIBRTLSDR_FOUND)
	include_directories(${LIBRTLSDR_INCLUDE_DIRS})
endif (LIBRTLSDR_FOUND)

find_package(LibLiquidSDR)
if (LIBLIQUIDSDR_FOUND)
	include_directories(${LIBLIQUIDSDR_INCLUDE_DIR})
endif (LIBLIQUIDSDR_FOUND)

# Don't look for librtfilter if libliquid available
if (NOT LIBLIQUIDSDR_FOUND)
	find_package(LibRTFilter)
	if (LIBRTFILTER_FOUND)
		include_directories(${LIBRTFILTER_INCLUDE_DIR})
	endif (LIBRTFILTER_FOUND)
endif (NOT LIBLIQUIDSDR_FOUND)

find_package(LibSSH)
if (LIBSSH_FOUND)
	include_directories(${LIBSSH_INCLUDE_DIR})
endif (LIBSSH_FOUND)

find_package(LibCurl)
if (LIBCURL_FOUND)
	include_directories(${LIBCURL_INCLUDE_DIR})
endif (LIBCURL_FOUND)

list(APPEND QRSSPIG_SOURCES main.cpp QrssPiG.cpp QGInputDevice.cpp QGInputStdIn.cpp QGProcessor.cpp QGImage.cpp QGUploader.cpp QGUploaderLocal.cpp)

if (LIBALSA_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputAlsa.cpp)
endif (LIBALSA_FOUND)

if (LIBHACKRF_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputHackRF.cpp)
endif (LIBHACKRF_FOUND)

if (LIBRTLSDR_FOUND)
	list(APPEND QRSSPIG_SOURCES QGInputRtlSdr.cpp)
endif (LIBRTLSDR_FOUND)

if (LIBSSH_FOUND)
	list(APPEND QRSSPIG_SOURCES QGUploaderSCP.cpp)
endif (LIBSSH_FOUND)

if (LIBCURL_FOUND)
	list(APPEND QRSSPIG_SOURCES QGUploaderFTP.cpp)
endif (LIBCURL_FOUND)

add_executable (qrsspig ${QRSSPIG_SOURCES})

target_link_libraries(qrsspig pthread)
target_link_libraries(qrsspig boost_program_options)
target_link_libraries(qrsspig yaml-cpp)
target_link_libraries(qrsspig fftw3f)
target_link_libraries(qrsspig gd)

if (LIBALSA_FOUND)
	target_link_libraries(qrsspig ${LIBALSA_LIBRARY})
	set(HAVE_LIBALSA 1)
endif (LIBALSA_FOUND)

if (LIBHACKRF_FOUND)
	target_link_libraries(qrsspig ${LIBHACKRF_LIBRARY})
	set(HAVE_LIBHACKRF 1)
endif (LIBHACKRF_FOUND)

if (LIBRTLSDR_FOUND)
	target_link_libraries(qrsspig ${LIBRTLSDR_LIBRARY})
	set(HAVE_LIBRTLSDR 1)
endif (LIBRTLSDR_FOUND)

if (LIBLIQUIDSDR_FOUND)
	target_link_libraries(qrsspig ${LIBLIQUIDSDR_LIBRARY})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LIBLIQUIDSDR_FOUND)

if (LIBRTFILTER_FOUND)
	target_link_libraries(qrsspig ${LIBRTFILTER_LIBRARY})
	set(HAVE_LIBRTFILTER 1)
endif (LIBRTFILTER_FOUND)

if (LIBSSH_FOUND)
	target_link_libraries(qrsspig ${LIBSSH_LIBRARY})
	set(HAVE_LIBSSH 1)
endif (LIBSSH_FOUND)

if (LIBCURL_FOUND)
	target_link_libraries(qrsspig ${LIBCURL_LIBRARY})
	set(HAVE_LIBCURL 1)
endif (LIBCURL_FOUND)

add_executable (IQGen IQGen.cpp)
target_link_libraries(IQGen boost_program_options)

add_executable (TestDownSampler TestDownSampler.cpp QGProcessor.cpp)
if (LIBLIQUIDSDR_FOUND)
	target_link_libraries(TestDownSampler ${LIBLIQUIDSDR_LIBRARY})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LIBLIQUIDSDR_FOUND)
if (LIBRTFILTER_FOUND)
	target_link_libraries(TestDownSampler ${LIBRTFILTER_LIBRARY})
	set(HAVE_LIBRTFILTER 1)
endif (LIBRTFILTER_FOUND)

# Generate and include Config.h
configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")
include_directories("${PROJECT_BINARY_DIR}")

install(TARGETS qrsspig RUNTIME DESTINATION bin)
