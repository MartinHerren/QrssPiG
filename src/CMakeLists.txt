# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_definitions(-std=c++11)

# Look for optional packages
find_package(LibRTFilter)
if (LIBRTFILTER_FOUND)
	include_directories(${LIBRTFILTER_INCLUDE_DIR})
endif (LIBRTFILTER_FOUND)

find_package(LibLiquidSDR)
if (LIBLIQUIDSDR_FOUND)
	include_directories(${LIBLIQUIDSDR_INCLUDE_DIR})
endif (LIBLIQUIDSDR_FOUND)

find_package(LibCurl)
if (LIBCURL_FOUND)
	include_directories(${LIBCURL_INCLUDE_DIR})
endif (LIBCURL_FOUND)

add_executable (qrsspig main.cpp QrssPiG.cpp QGDownSampler.cpp QGFft.cpp QGImage.cpp QGUploader.cpp QGLocalUploader.cpp QGSCPUploader.cpp QGUploaderFTP.cpp)
target_link_libraries(qrsspig pthread)
target_link_libraries(qrsspig boost_program_options)
target_link_libraries(qrsspig fftw3f)
target_link_libraries(qrsspig gd)
target_link_libraries(qrsspig ssh)
target_link_libraries(qrsspig yaml-cpp)
if (LIBRTFILTER_FOUND)
	target_link_libraries(qrsspig ${LIBRTFILTER_LIBRARY})
	set(HAVE_LIBRTFILTER 1)
endif (LIBRTFILTER_FOUND)
if (LIBLIQUIDSDR_FOUND)
	target_link_libraries(qrsspig ${LIBLIQUIDSDR_LIBRARY})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LIBLIQUIDSDR_FOUND)
if (LIBCURL_FOUND)
	target_link_libraries(qrsspig ${LIBCURL_LIBRARY})
	set(HAVE_LIBCURL 1)
endif (LIBCURL_FOUND)

add_executable (IQGen IQGen.cpp)
target_link_libraries(IQGen boost_program_options)

add_executable (TestDownSampler TestDownSampler.cpp QGDownSampler.cpp)
if (LIBRTFILTER_FOUND)
	target_link_libraries(TestDownSampler ${LIBRTFILTER_LIBRARY})
	set(HAVE_LIBRTFILTER 1)
endif (LIBRTFILTER_FOUND)
if (LIBLIQUIDSDR_FOUND)
	target_link_libraries(TestDownSampler ${LIBLIQUIDSDR_LIBRARY})
	set(HAVE_LIBLIQUIDSDR 1)
endif (LIBLIQUIDSDR_FOUND)

# Generate and include Config.h
configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")
include_directories("${PROJECT_BINARY_DIR}")

install(TARGETS qrsspig RUNTIME DESTINATION bin)
