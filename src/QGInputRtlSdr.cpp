#include "QGInputRtlSdr.h"

#include <iostream>
#include <stdexcept>

QGInputRtlSdr::QGInputRtlSdr(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
}

QGInputRtlSdr::~QGInputRtlSdr() {
	if (_device) {
		rtlsdr_close(_device);
	}
}

void QGInputRtlSdr::open() {
	int index = 0;

	std::cout << "Devices: " << rtlsdr_get_device_count() << std::endl;

	std::cout << "Version: " << rtlsdr_get_device_name(index) << std::endl;

	rtlsdr_open(&_device, index);

	if (rtlsdr_set_sample_rate(_device, _sampleRate)) {
		std::cout << "Failed setting samplerate" << std::endl;
	};
	std::cout << rtlsdr_get_sample_rate(_device) << std::endl;

	if (rtlsdr_set_center_freq(_device, _baseFreq)) {
		std::cout << "Failed setting frequency" << std::endl;
	};
	std::cout << rtlsdr_get_center_freq(_device) << std::endl;
}
