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

	_readBufferSize = 8192;
	_readBuffer.reset(new char[_readBufferSize]);
}

QGInputStdIn::~QGInputStdIn() {
	// If stopping because cin ran out, stop() hasn't been called and exited thread must still be joined
	if (_t.joinable()) _t.join();
}

void QGInputStdIn::_startDevice() {
	_t = std::thread(std::bind(&QGInputStdIn::_process, this));
}

void QGInputStdIn::_stopDevice() {
	if (_t.joinable()) _t.join();
}

void QGInputStdIn::_process() {
	std::cin >> std::noskipws;

	while (std::cin && _running) {
		std::cin.read(_readBuffer.get(), _readBufferSize);

		switch (_format) {
			case Format::U8IQ:
				if (_bufferSize + _readBufferSize/2 <= _bufferCapacity) {
					unsigned char i, q;

					for (int j = 0; j < _readBufferSize;) {
						i = _readBuffer[j++];
						q = _readBuffer[j++];
						_buffer[_bufferHead++] = std::complex<float>((i - 128) / 128., (q - 128) / 128.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/2;
				} else {
					std::this_thread::sleep_for(std::chrono::microseconds(1000000));
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::S8IQ:
				if (_bufferSize + _readBufferSize/2 <= _bufferCapacity) {
					signed char i, q;

					for (int j = 0; j < _readBufferSize;) {
						i = _readBuffer[j++];
						q = _readBuffer[j++];
						_buffer[_bufferHead++] = std::complex<float>(i / 128., q / 128.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/2;
				} else {
					std::this_thread::sleep_for(std::chrono::microseconds(1000000));
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::U16IQ:
				if (_bufferSize + _readBufferSize/4 <= _bufferCapacity) {
					unsigned short int i, q;

					for (int j = 0; j < _readBufferSize;) {
						i = _readBuffer[j++];
						i += _readBuffer[j++] << 8;
						q = _readBuffer[j++];
						q += _readBuffer[j++] << 8;
						_buffer[_bufferHead++] = std::complex<float>((i - 32768) / 32768., (q - 32768) / 32768.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/4;
				} else {
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::S16IQ:
				if (_bufferSize + _readBufferSize/4 <= _bufferCapacity) {
					signed short int i, q;

					for (int j = 0; j < _readBufferSize;) {
						i = _readBuffer[j++];
						i += _readBuffer[j++] << 8;
						q = _readBuffer[j++];
						q += _readBuffer[j++] << 8;
						_buffer[_bufferHead++] = std::complex<float>(i / 32768., q / 32768.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/4;
				} else {
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::S16MONO:
				if (_bufferSize + _readBufferSize/4 <= _bufferCapacity) {
					signed short int r;

					for (int j = 0; j < _readBufferSize;) {
						r = _readBuffer[j++];
						r += _readBuffer[j++] << 8;
						_buffer[_bufferHead++] = std::complex<float>(r / 32768., 0.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/2;
				} else {
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::S16LEFT:
				if (_bufferSize + _readBufferSize/4 <= _bufferCapacity) {
					signed short int r;

					for (int j = 0; j < _readBufferSize;) {
						r = _readBuffer[j++];
						r += _readBuffer[j++] << 8;
						j += 2;
						_buffer[_bufferHead++] = std::complex<float>(r / 32768., 0.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/4;
				} else {
					std::cout << "drop" << std::endl;
				}
				break;

			case Format::S16RIGHT:
				if (_bufferSize + _readBufferSize/4 <= _bufferCapacity) {
					signed short int r;

					for (int j = 0; j < _readBufferSize;) {
						j += 2;
						r = _readBuffer[j++];
						r += _readBuffer[j++] << 8;
						_buffer[_bufferHead++] = std::complex<float>(r / 32768., 0.);
						_bufferHead %= _bufferCapacity;
					}

					_bufferSize += _readBufferSize/4;
				} else {
					std::cout << "drop" << std::endl;
				}
				break;
		}
	}

	// In case we cabe here because of std::cin running out, this will signal main loop to finish
	_running = false;
}
