#include "QGInputHackRF.h"

#include <iostream>
#include <stdexcept>

QGInputHackRF::QGInputHackRF(): _hackrf(nullptr) {
}

QGInputHackRF::~QGInputHackRF() {
}

void QGInputHackRF::open() {
	int index = 0;
	int _sampleRate = 0;
	int _baseFreq = 0;

       	int r = hackrf_init();

	if (r != HACKRF_SUCCESS) {
		std::cout << "HackRF init returned " << r <<
		std::endl;
	}

	hackrf_device_list_t *l = hackrf_device_list();

	std::cout << "Devices: " << l->devicecount << std::endl;

	r = hackrf_device_list_open(l, index, &_hackrf);

	if (r != HACKRF_SUCCESS) {
		_hackrf = nullptr; std::cout << "HackRF opening failed: " << r << std::endl;
		hackrf_exit();
		return;
	}

	hackrf_device_list_free(l);

	char v[255]; uint8_t vl = 255; hackrf_version_string_read(_hackrf, v, vl);

	std::cout << "Version: " << v << std::endl;

	r = hackrf_set_sample_rate(_hackrf, _sampleRate);

	if (r != HACKRF_SUCCESS) {
		std::cout << "HackRF setting samplerate failed: " << r << std::endl;
	}

	r = hackrf_set_freq(_hackrf, _baseFreq);

	if (r != HACKRF_SUCCESS) {
		std::cout << "HackRF setting frequency failed: " << r << std::endl;
	}

	r = hackrf_close(_hackrf);

	if (r != HACKRF_SUCCESS) {
		std::cout << "HackRF closing failed: " << r << std::endl;
	}

	_hackrf = nullptr;

	hackrf_exit();
}
