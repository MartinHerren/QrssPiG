#include "QGInputHackRF.h"

#include <iostream>
#include <stdexcept>

QGInputHackRF::QGInputHackRF(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	int r = hackrf_init();

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF init returned ") + std::to_string(r));
	}

	//r = hackrf_device_list_open(l, _deviceIndex, &_device);
	r = hackrf_open(&_device);

	if (r != HACKRF_SUCCESS) {
		_device = nullptr;
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF opening failed: ") + std::to_string(r));
	}

	char v[255]; uint8_t vl = 255;
	hackrf_version_string_read(_device, v, vl);

	std::cout << "Version: " << v << std::endl;

	r = hackrf_set_sample_rate(_device, _sampleRate);

	if (r != HACKRF_SUCCESS) {
		_device = nullptr;
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting samplerate failed: ") + std::to_string(r));
	}

	r = hackrf_set_freq(_device, _baseFreq);

	if (r != HACKRF_SUCCESS) {
		_device = nullptr;
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting frequency failed: ") + std::to_string(r));
	}
}

QGInputHackRF::~QGInputHackRF() {
	if (_device) {
		int r = hackrf_close(_device);

		if (r != HACKRF_SUCCESS) {
			std::cout << "HackRF closing failed: " << r << std::endl;
		}
	}

	hackrf_exit();
}

void QGInputHackRF::deviceList() {
	hackrf_device_list_t *l = hackrf_device_list();

	std::cout << "Devices: " << l->devicecount << std::endl;

	hackrf_device_list_free(l);
}

void QGInputHackRF::run(std::function<void(std::complex<float>)>cb) {
}

void QGInputHackRF::stop() {
}
