set(Module "upload_local")

add_library(${Module} SHARED
	QGUploaderLocal.cpp
)

target_link_libraries(${Module} ${LibSSH_LIBRARIES})

install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)
