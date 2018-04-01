find_package(LibAlsa)

if (LibAlsa_FOUND)
	
	set(Module "input_alsa")

	add_library(${Module} SHARED
		QGInputAlsa.cpp
	)

	target_include_directories(${Module} PUBLIC ${LibAlsa_INCLUDE_DIRS})
	target_link_libraries(${Module} ${LibAlsa_LIBRARIES})

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

	set(HAVE_LIBALSA 1)
endif (LibAlsa_FOUND)
