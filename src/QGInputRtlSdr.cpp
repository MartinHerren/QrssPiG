#include "QGInputRtlSdr.h"

#include <iostream>
#include <stdexcept>

QGInputRtlSdr::QGInputRtlSdr(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	_deviceIndex = 0;

	if (config["deviceindex"]) _deviceIndex = config["deviceindex"].as<int>();

	std::cout << "Opening rtlsdr: " << rtlsdr_get_device_name(_deviceIndex) << std::endl;

	rtlsdr_open(&_device, _deviceIndex);

	if (rtlsdr_set_sample_rate(_device, _sampleRate)) {
		throw std::runtime_error("Failed setting samplerate");
	}

	// Save back effective rate and freq to be retrieved by core
	std::cout << "Requested sample rate: " << _sampleRate << std::endl;
	_sampleRate = rtlsdr_get_sample_rate(_device);
	std::cout << "Effective sample rate: " << _sampleRate << std::endl;

	if (rtlsdr_set_center_freq(_device, _baseFreq)) {
		throw std::runtime_error("Failed setting frequency");
	};

	std::cout << "Requested frequency: " << _baseFreq << std::endl;
	_baseFreq = rtlsdr_get_center_freq(_device);
	std::cout << "Effective frequency: " << _baseFreq << std::endl;

	if (rtlsdr_set_freq_correction(_device, _ppm)) {
		throw std::runtime_error("Failed setting ppm");
	}

	// Set ppm to 0 to mark as 'consumed', so that output image will not correct it a second time
	_ppm = 0;
}

QGInputRtlSdr::~QGInputRtlSdr() {
	if (_device) {
		rtlsdr_close(_device);
	}
}

void QGInputRtlSdr::deviceList() {
	std::cout << "Devices: " << rtlsdr_get_device_count() << std::endl;
}

void QGInputRtlSdr::run(std::function<void(std::complex<float>)>cb) {
}

void QGInputRtlSdr::stop() {
}
