#include "QGInputHackRF.h"

#include "Config.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

std::vector<std::string> QGInputHackRF::listDevices() {
	std::vector<std::string> list;

#ifdef HAVE_LIBHACKRF_DEVICE_LIST
	hackrf_device* device;
	int r = hackrf_init();

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF init returned ") + std::to_string(r));
	}

	hackrf_device_list_t *l = hackrf_device_list();

	for (int i = 0; i < l->devicecount; i++) {
		if ((r = hackrf_device_list_open(l, i, &device)) == 0) {
			unsigned char boardId;
			std::unique_ptr<char[]> version(new char[255]);
			hackrf_board_id_read(device, &boardId);
			hackrf_version_string_read(device, version.get(), 255);
			list.push_back(std::to_string(i) + "\t" + hackrf_board_id_name((hackrf_board_id)boardId) + " v" + version.get() + " (serial: " + l->serial_numbers[i] + ")");
			hackrf_close(device);
		} else {
			list.push_back(std::to_string(i) + "\tError opening device: " + hackrf_error_name((hackrf_error)r));
		}
	}

	hackrf_device_list_free(l);

	hackrf_exit();
#else
	list.push_back("Device listing not supported by this libhackrf version");
#endif // HAVE_LIBHACKRF_DEVICE_LIST

	return list;
}

QGInputHackRF::QGInputHackRF(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	unsigned char antennaPowerEnabled = 0;
	unsigned char ampEnabled = 0;
	unsigned int lnaGain = 16;
	unsigned int vgaGain = 16;

	if (config["antennapower"]) antennaPowerEnabled = config["antennapower"].as<bool>() ? 1 : 0;
	if (config["amplifier"]) ampEnabled = config["amplifier"].as<bool>() ? 1 : 0;
	if (config["lnagain"]) lnaGain = config["lnagain"].as<unsigned int>();
	if (config["vgagain"]) vgaGain = config["vgagain"].as<unsigned int>();

	if (lnaGain > 40) lnaGain = 40;
	lnaGain &= 0x38;
	if (vgaGain > 62) vgaGain = 62;
	vgaGain &= 0x3e;

	int r = hackrf_init();

	if (r != HACKRF_SUCCESS) {
		throw std::runtime_error(std::string("HackRF init returned ") + std::to_string(r));
	}

	//r = hackrf_device_list_open(l, _deviceIndex, &_device);
	r = hackrf_open(&_device);

	if (r != HACKRF_SUCCESS) {
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF opening failed: ") + std::to_string(r));
	}

	r = hackrf_set_sample_rate(_device, _sampleRate);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting samplerate failed: ") + std::to_string(r));
	}

	r = hackrf_set_freq(_device, _baseFreq);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting frequency failed: ") + std::to_string(r));
	}

	r = hackrf_set_antenna_enable(_device, antennaPowerEnabled);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting antenna power failed: ") + std::to_string(r));
	}

	r = hackrf_set_amp_enable(_device, ampEnabled);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting amplifier failed: ") + std::to_string(r));
	}

	r = hackrf_set_lna_gain(_device, lnaGain);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting LNA gain failed: ") + std::to_string(r));
	}

	r = hackrf_set_vga_gain(_device, vgaGain);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting VGA gain failed: ") + std::to_string(r));
	}

	r = hackrf_set_baseband_filter_bandwidth(_device, 1750000);

	if (r != HACKRF_SUCCESS) {
		hackrf_close(_device);
		hackrf_exit();
		throw std::runtime_error(std::string("HackRF setting baseband filter failed: ") + std::to_string(r));
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

	_bufferSize += len/2;
}

int QGInputHackRF::async(hackrf_transfer* transfer) {
	try {
		static_cast<QGInputHackRF *>(transfer->rx_ctx)->_process(transfer->buffer, transfer->valid_length);
	} catch (const std::exception &e) {
		// TODO return error,  reset device if error is from device read and ignore if from fft ?
	}

	return HACKRF_SUCCESS;
}
