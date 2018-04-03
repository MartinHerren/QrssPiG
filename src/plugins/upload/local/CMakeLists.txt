set(Module "upload_local")

add_library(${Module} SHARED
	QGUploaderLocal.cpp
)

install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)
