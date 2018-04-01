
find_package(LibCurl)

if (LibCurl_FOUND)

	set(Module "upload_ftp")

	include_directories(${LibCurl_INCLUDE_DIRS})

	add_library(${Module} SHARED
		QGUploaderFTP.cpp
	)

	target_link_libraries(${Module} ${LibSSH_LIBRARIES})

	install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)

	set(HAVE_LIBCURL 1)

endif (LibCurl_FOUND)
