#include "QGInputStdIn.h"

#include <iostream>
#include <stdexcept>

QGInputStdIn::QGInputStdIn(const YAML::Node &config): QGInputDevice(config) {
	_format = Format::U8IQ;
	
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

void QGInputStdIn::run(std::function<void(std::complex<float>)>cb) {
	char b[8192];
	_running = true;
	std::cin >> std::noskipws;

	switch (_format) {
		case Format::U8IQ: {
			unsigned char i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					q = b[j++];
					cb(std::complex<float>((i - 128) / 128., (q - 128) / 128.));
				}
			}
			break;
		}

		case Format::S8IQ: {
			signed char i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					q = b[j++];
					cb(std::complex<float>(i / 128., q / 128.));
				}
			}
			break;
		}

		case Format::U16IQ: {
			unsigned short int i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					i += b[j++] << 8;
					q = b[j++];
					q += b[j++] << 8;
					cb(std::complex<float>((i - 32768) / 32768., (q - 32768) / 32768.));
				}
			}
			break;
		}

		case Format::S16IQ: {
			signed short int i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					i += b[j++] << 8;
					q = b[j++];
					q += b[j++] << 8;
					cb(std::complex<float>(i / 32768., q / 32768.));
				}
			}
			break;
		}

		case Format::S16MONO: {
			signed short int r;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					r = b[j++];
					r += b[j++] << 8;
					cb(std::complex<float>(r / 32768., 0.));
				}
			}
		}

		case Format::S16LEFT: {
			signed short int r;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					r = b[j++];
					r += b[j++] << 8;
					j += 2;
					cb(std::complex<float>(r / 32768., 0.));
				}
			}
		}

		case Format::S16RIGHT: {
			signed short int r;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					j += 2;
					r = b[j++];
					r += b[j++] << 8;
					cb(std::complex<float>(r / 32768., 0.));
				}
			}
		}
	}
}

void QGInputStdIn::stop() {
	_running = false;
}
