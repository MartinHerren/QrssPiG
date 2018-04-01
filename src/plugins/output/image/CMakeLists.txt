find_package(LibGd REQUIRED)

if (LibGd_FOUND)
	set(Module "output_image")

	add_library(${Module} SHARED
		QGImage.cpp
	)

	target_include_directories(${Module} PUBLIC ${LibGd_INCLUDE_DIRS})
	target_link_libraries(${Module} ${LibGd_LIBRARIES})

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

endif (LibGd_FOUND)
