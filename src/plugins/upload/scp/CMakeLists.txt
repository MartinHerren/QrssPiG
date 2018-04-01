find_package(LibSSH)

if (LibSSH_FOUND)
	
	set(Module "upload_scp")

	include_directories(${LibSSH_INCLUDE_DIRS})

	add_library(${Module} SHARED
		QGUploaderSCP.cpp
	)

	target_link_libraries(${Module} ${LibSSH_LIBRARIES})

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

	set(HAVE_LIBSSH 1)
endif (LibSSH_FOUND)
