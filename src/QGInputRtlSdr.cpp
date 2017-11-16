#include "QGInputRtlSdr.h"

#include <iostream>
#include <memory>
#include <stdexcept>

std::vector<std::string> QGInputRtlSdr::listDevices() {
	std::vector<std::string> list;
	unsigned int c = rtlsdr_get_device_count();

	std::cout << "Devices: " << c << std::endl;

	for (unsigned int i = 0; i < c; i++) {
		std::unique_ptr<char[]> manufacturer(new char[256]);
		std::unique_ptr<char[]> product(new char[256]);
		std::unique_ptr<char[]> serial(new char[256]);
		rtlsdr_get_device_usb_strings(i, manufacturer.get(), product.get(), serial.get());
		const char *name = rtlsdr_get_device_name(i);
		list.push_back(std::to_string(i) + ": \"" + manufacturer.get() + "\" \"" + product.get() + "\" \"" + serial.get() + "\" - " + name);
	}

	return list;
}

QGInputRtlSdr::QGInputRtlSdr(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	_deviceIndex = 0;

	if (config["deviceindex"]) _deviceIndex = config["deviceindex"].as<int>();

	std::cout << "Opening rtlsdr: " << rtlsdr_get_device_name(_deviceIndex) << std::endl;

	if (rtlsdr_open(&_device, _deviceIndex)) {
		throw std::runtime_error("Failed to open device");
	}

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
	// TODO substract from get_freq_correction ?
	_ppm = 0;

	// TODO: Check in cmake if function exists in installed librtlsdr version
	//if (rtlsdr_set_tuner_bandwidth(_device, 350000)) throw std::runtime_error("Failed setting bandwith");
}

QGInputRtlSdr::~QGInputRtlSdr() {
	if (_device) rtlsdr_close(_device);
}

void QGInputRtlSdr::_startDevice() {
	if (rtlsdr_reset_buffer(_device)) throw std::runtime_error("Error reseting device");
	_t = std::thread(rtlsdr_read_async, _device, &this->async, this, 0, 0);

	return;
}

void QGInputRtlSdr::_stopDevice() {
	rtlsdr_cancel_async(_device);
	_t.join();
}

void QGInputRtlSdr::_process(unsigned char *buf, uint32_t len) {
	// U8 IQ data
	unsigned char i, q;

	// Drop new data if buffer full
	if (_bufferSize + len/2 > _bufferCapacity) {
		std::cout << "drop" << std::endl;
		return;
	}

	for (unsigned int j = 0; j < len;) {
		i = buf[j++];
		q = buf[j++];
		_buffer[_bufferHead++] = std::complex<float>((i - 128) / 128., (q - 128) / 128.);
		_bufferHead %= _bufferCapacity;
	}

	_bufferSize += len/2;
}

void QGInputRtlSdr::async(unsigned char *buf, uint32_t len, void *ctx) {
	try {
		static_cast<QGInputRtlSdr *>(ctx)->_process(buf, len);
	} catch (const std::exception &e) {
		// TODO reset device  if error is from rtl device read and ignore if from fft ?
	}
}
