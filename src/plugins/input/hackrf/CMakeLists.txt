find_package(LibHackRF)

if (LibHackRF_FOUND)
	
	set(Module "input_hackrf")

	add_library(${Module} SHARED
		QGInputHackRF.cpp
	)

	target_include_directories(${Module} PUBLIC ${LibHackRF_INCLUDE_DIRS})
	target_link_libraries(${Module} ${LibHackRF_LIBRARIES})

	if (LibHackRF_HAS_hackrf_device_list)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LIBHACKRF_DEVICE_LIST")
	endif (LibHackRF_HAS_hackrf_device_list)

	if (LibHackRF_HAS_hackrf_set_antenna_enable)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LIBHACKRF_ANTENNA_ENABLE")
	endif (LibHackRF_HAS_hackrf_set_antenna_enable)

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

	set(HAVE_LIBHACKRF 1)
endif (LibHackRF_FOUND)
