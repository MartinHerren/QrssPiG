#include "QGInputRtlSdr.h"

#include "Config.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

extern "C" QGInputDevice* create_object(const YAML::Node &config) {
        return new QGInputRtlSdr(config);
}

extern "C" std::vector<std::string> list_devices() { 
        return QGInputRtlSdr::listDevices();
}

std::vector<std::string> QGInputRtlSdr::listDevices() {
	std::vector<std::string> list;

#ifdef HAVE_LIBRTLSDR_DEVICE_LIST
	unsigned int c = rtlsdr_get_device_count();

	for (unsigned int i = 0; i < c; i++) {
		std::unique_ptr<char[]> manufacturer(new char[256]);
		std::unique_ptr<char[]> product(new char[256]);
		std::unique_ptr<char[]> serial(new char[256]);
		rtlsdr_get_device_usb_strings(i, manufacturer.get(), product.get(), serial.get());
		const char *name = rtlsdr_get_device_name(i);
		list.push_back(std::to_string(i) + "\t" + manufacturer.get() + " " + product.get() + " - " + name + " (serial: " + serial.get() + ")");
	}
#else
	list.push_back("Device listing not supported by this librtlsdr version");
#endif // HAVE_LIBRTLSDR_DEVICE_LIST

	return list;
}

QGInputRtlSdr::QGInputRtlSdr(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	_type = "RtlSdr";
	int deviceIndex = 0;
	float gain = 24.;
	DirectSampling directsampling = DirectSampling::OFF;
	unsigned int bandwidth = 0;


	if (config["deviceindex"]) deviceIndex = config["deviceindex"].as<int>();
	if (config["gain"]) gain = config["gain"].as<float>();
	if (config["directsampling"]) {
		if (config["directsampling"].as<bool>()) {
			if (config["directsamplingbranch"]) {
				std::string b = config["directsamplingbranch"].as<std::string>();

				if (b.compare("i-branch") == 0) directsampling = DirectSampling::I_BRANCH;
				if (b.compare("q-branch") == 0) directsampling = DirectSampling::Q_BRANCH;
				else throw std::runtime_error("YAML: directsamplingbranch value unrecognized");
			} else {
				// Defaulting to Q-Branch as it is the branch in rtlsdr v3 dongles
				directsampling = DirectSampling::Q_BRANCH;
			}
		} else {
			directsampling = DirectSampling::OFF;
		}
	}
	if (config["bandwidth"]) bandwidth = config["bandwidth"].as<unsigned int>();

	std::cout << "Opening rtlsdr: " << rtlsdr_get_device_name(deviceIndex) << std::endl;

	if (rtlsdr_open(&_device, deviceIndex)) {
		throw std::runtime_error("Failed to open device");
	}

	if (rtlsdr_set_sample_rate(_device, _sampleRate)) {
		throw std::runtime_error("Failed setting samplerate");
	}

	// Do it first, as it might influence rtlsdr_set_center_freq()'s inner working'
	if (rtlsdr_set_direct_sampling(_device, (int)directsampling)) {
		throw std::runtime_error("Failed setting directsampling");
	}

	// Save back effective rate and freq to be retrieved by core
	std::cout << "Requested sample rate: " << _sampleRate << std::endl;
	_sampleRate = rtlsdr_get_sample_rate(_device);
	std::cout << "Effective sample rate: " << _sampleRate << std::endl;

	if (rtlsdr_set_center_freq(_device, _baseFreq)) {
		throw std::runtime_error("Failed setting frequency");
	}

	std::cout << "Requested frequency: " << _baseFreq << std::endl;
	_baseFreq = rtlsdr_get_center_freq(_device);
	std::cout << "Effective frequency: " << _baseFreq << std::endl;

	if (_residualPpm != 0.) {
		int ppmInt = trunc(_residualPpm);

		if (rtlsdr_set_freq_correction(_device, ppmInt)) {
			throw std::runtime_error("Failed setting ppm");
		}

		_residualPpm -= ppmInt;
	}

	if (bandwidth != 0) {
#ifdef HAVE_LIBRTLSDR_TUNER_BANDWITH
		if (rtlsdr_set_tuner_bandwidth(_device, bandwidth)) {
			throw std::runtime_error("Failed setting tuner bandwith");
		}
#else
		std::cout << "Warning:\ttuner bandwidth option not available for installed librtlsdr version" << std::endl;
#endif // HAVE_LIBRTLSDR_TUNER_BANDWITH
	}

	if (rtlsdr_set_tuner_gain(_device, _validGain(gain))) {
		throw std::runtime_error("Failed setting gain");
	}

	std::cout << "Requested gain: " << gain << "dB" << std::endl;
	std::cout << "Effective gain: " << (rtlsdr_get_tuner_gain(_device) / 10.) << "dB" << std::endl;
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

int QGInputRtlSdr::_validGain(float gain) {
	int numGains = rtlsdr_get_tuner_gains(_device, nullptr);
	std::unique_ptr<int[]> gains(new int[numGains]);
	rtlsdr_get_tuner_gains(_device, gains.get());

	// Get highest gain not higher than requested gain. Gains returned by api are in tenth of dB
	for (int i = 1; i < numGains; i++) {
		if ((gain * 10.) < gains[i]) return gains[i - 1];
	}

	return gains[numGains - 1];
}

void QGInputRtlSdr::async(unsigned char *buf, uint32_t len, void *ctx) {
	try {
		static_cast<QGInputRtlSdr *>(ctx)->_process(buf, len);
	} catch (const std::exception &e) {
		// TODO reset device  if error is from rtl device read and ignore if from fft ?
	}
}
