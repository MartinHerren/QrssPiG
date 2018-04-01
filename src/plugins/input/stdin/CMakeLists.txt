set(Module "input_stdin")

add_library(${Module} SHARED
	QGInputStdIn.cpp
)

install(TARGETS ${Module} LIBRARY DESTINATION lib/qrsspig)
