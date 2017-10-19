#include "QGInputStdIn.h"

#include <iostream>
#include <stdexcept>

QGInputStdIn::QGInputStdIn(const YAML::Node &config): QGInputDevice(config) {
	if (config["format"]) {
		std::string f = config["format"].as<std::string>();

		if ((f.compare("u8iq") == 0) || (f.compare("rtlsdr") == 0)) _format = Format::U8IQ;
		else if ((f.compare("s8iq") == 0) || (f.compare("hackrf") == 0)) _format = Format::S8IQ;
		else if (f.compare("u16iq") == 0) _format = Format::U16IQ;
		else if (f.compare("s16iq") == 0) _format = Format::S16IQ;
		else if (f.compare("s16mono") == 0) _format = Format::S16MONO;
		else if (f.compare("s16left") == 0) _format = Format::S16LEFT;
		else if (f.compare("s16right") == 0) _format = Format::S16RIGHT;
		else throw std::runtime_error("YAML: input format unrecognized");
	}
}

QGInputStdIn::~QGInputStdIn() {
}

void QGInputStdIn::open() {
}
