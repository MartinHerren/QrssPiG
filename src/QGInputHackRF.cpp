#include "QGInputHackRF.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

QGInputHackRF::QGInputHackRF(const YAML::Node &config, std::function<void(const std::complex<float>*, unsigned int)>cb): QGInputDevice(config, cb), _device(nullptr) {
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

	_bufferCapacity = 10 * 32 * 262144; // ~10 seconds of buffer @ 8MS/s
	_buffer.resize(_bufferCapacity);
	_bufferSize = 0;
	_bufferHead = 0;
	_bufferTail = 0;
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

void QGInputHackRF::run() {
	_running.lock(); // lock to running state.

	int r = hackrf_start_rx(_device, async, this);

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF run failed: ") + std::to_string(r));
	}

	while (!_running.try_lock()) { // loop until lock as been freed by stop()
		if (_bufferSize > 8192*10) {
			for (int j = 0; j < 8192*10; j += 32) {
				_cb(&_buffer[_bufferTail], 32);
				_bufferTail += 32;
				_bufferTail %= _bufferCapacity;
			}

			// Update size
			_bufferMutex.lock();
			_bufferSize -= 8192*10;
			_bufferMutex.unlock();
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
	_running.unlock(); // Cleanup

	return;
}

void QGInputHackRF::stop() {
	if (!_running.try_lock()) {
		int r = hackrf_stop_rx(_device);

		if (r != HACKRF_SUCCESS) {
			throw std::runtime_error(std::string("HackRF stop failed: ") + std::to_string(r));
		}
	}

	_running.unlock();// Enable run() to return, or release the lock we just gained if it was not running
}

void QGInputHackRF::process(uint8_t *buf, int len) {
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

	// Update size
	_bufferMutex.lock();
	_bufferSize += len/2;
	_bufferMutex.unlock();
}

int QGInputHackRF::async(hackrf_transfer* transfer) {
	try {
		static_cast<QGInputHackRF *>(transfer->rx_ctx)->process(transfer->buffer, transfer->valid_length);
	} catch (const std::exception &e) {
		// TODO return error,  reset device if error is from device read and ignore if from fft ?
	}

	return HACKRF_SUCCESS;
}
