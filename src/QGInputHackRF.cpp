#include "QGInputHackRF.h"

#include <chrono>
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
	int r = hackrf_init();

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF init returned ") + std::to_string(r));
	}

	hackrf_device_list_t *l = hackrf_device_list();

	std::cout << "Devices: " << l->devicecount << std::endl;

	hackrf_device_list_free(l);

	hackrf_exit();
}

void QGInputHackRF::_startDevice() {
	int r = hackrf_start_rx(_device, async, this);

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF run failed: ") + std::to_string(r));
	}

	return;
}

void QGInputHackRF::_stopDevice() {
	int r = hackrf_stop_rx(_device);

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF stop failed: ") + std::to_string(r));
	}
}

void QGInputHackRF::_process(uint8_t *buf, int len) {
	// S8 IQ data
	signed char i, q;

	// Drop new data if buffer full
	if (_bufferSize + len/2 > _bufferCapacity) {
		std::cout << "drop" << std::endl;
		return;
	}

	for (int j = 0; j < len;) {
		i = buf[j++];
		q = buf[j++];
		_buffer[_bufferHead++] = std::complex<float>(i / 128., q / 128.);
		_bufferHead %= _bufferCapacity;
	}

	_adjBufferSize(len/2);
}

int QGInputHackRF::async(hackrf_transfer* transfer) {
	try {
		static_cast<QGInputHackRF *>(transfer->rx_ctx)->_process(transfer->buffer, transfer->valid_length);
	} catch (const std::exception &e) {
		// TODO return error,  reset device if error is from device read and ignore if from fft ?
	}

	return HACKRF_SUCCESS;
}
