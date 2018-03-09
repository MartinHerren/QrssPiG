find_package(LibRtlSdr)

if (LibRtlSdr_FOUND)
	
	set(Module "input_rtlsdr")

	add_library(${Module} SHARED
		QGInputRtlSdr.cpp
	)

	target_include_directories(${Module} PUBLIC ${LibRtlSdr_INCLUDE_DIRS})
	target_link_libraries(${Module} ${LibRtlSdr_LIBRARIES})

        if (LibRtlSdr_HAS_rtlsdr_get_device_count)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LIBRTLSDR_DEVICE_LIST")
        endif (LibRtlSdr_HAS_rtlsdr_get_device_count)
        if (LibRtlSdr_HAS_rtlsdr_set_tuner_bandwidth)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LIBRTLSDR_TUNER_BANDWITH")
        endif (LibRtlSdr_HAS_rtlsdr_set_tuner_bandwidth)

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

	set(HAVE_LIBRTLSDR 1)
endif (LibRtlSdr_FOUND)
