#include "QGInputAlsa.h"

#include <iostream>
#include <stdexcept>

QGInputAlsa::QGInputAlsa(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	_deviceName = "hw:0,0";
	_channel = Channel::LEFT;

	if (config["device"]) _deviceName = config["device"].as<std::string>();
	if (config["channel"]) {
		std::string c = config["channel"].as<std::string>();

		if (c.compare("mono") == 0) _channel = Channel::MONO;
		else if (c.compare("left") == 0) _channel = Channel::LEFT;
		else if (c.compare("right") == 0) _channel = Channel::RIGHT;
		else if (c.compare("iq") == 0) _channel = Channel::IQ;
		else if (c.compare("inviq") == 0) _channel = Channel::INVIQ;
	}

	// TODO depending on format
	_bytesPerSample = 2;

	switch (_channel) {
	case Channel::MONO:
		_numChannels = 1;
		break;
	case Channel::LEFT:
	case Channel::RIGHT:
	case Channel::IQ:
	case Channel::INVIQ:
		_numChannels = 2;
		break;
	}

	int err;
	snd_pcm_hw_params_t *hw_params;

	if ((err = snd_pcm_open(&_device, _deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0))) throw std::runtime_error(std::string("Error opening audio device: ") + snd_strerror(err));

	if ((err = snd_pcm_hw_params_malloc(&hw_params))) throw std::runtime_error(std::string("Error allocating audio device params: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_any(_device, hw_params))) throw std::runtime_error(std::string("Error getting audio device params: ") + snd_strerror(err));

	if ((err = snd_pcm_hw_params_set_access(_device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))) throw std::runtime_error(std::string("Error setting audio device access: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_set_format(_device, hw_params, SND_PCM_FORMAT_S16_LE))) throw std::runtime_error(std::string("Error setting audio device format: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_set_channels(_device, hw_params, _numChannels))) throw std::runtime_error(std::string("Error setting audio device channels: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_set_rate_near(_device, hw_params, &_sampleRate, nullptr))) throw std::runtime_error(std::string("Error setting audio device rate: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params(_device, hw_params))) throw std::runtime_error(std::string("Error setting audio device params: ") + snd_strerror(err));

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(_device))) throw std::runtime_error(std::string("Error preparing audio device: ") + snd_strerror(err));
}

QGInputAlsa::~QGInputAlsa() {
	if (_device) snd_pcm_close (_device);
}

void QGInputAlsa::run(std::function<void(std::complex<float>)>cb) {
	int samplesPerCall = 4096;
	int bufferSize = samplesPerCall * _bytesPerSample * _numChannels;
	unsigned char *b = static_cast<unsigned char*>(malloc(bufferSize));

	_running = true;

	int err;

	while (_running) {
		if ((err = snd_pcm_readi(_device, b, samplesPerCall)) != samplesPerCall) {
			if (err < 0) {
				throw std::runtime_error(std::string("Error reading from audio device: ") + snd_strerror(err));
			} else {
				samplesPerCall = err; // push only residual read
				_running = false;
			}
		}

		signed short int i = 0, q = 0;

		// Only S16_LE supported so far
		switch (_channel) {
		case Channel::MONO:
			for (int j = 0; j < bufferSize;) {
				i = b[j++];
				i += b[j++] << 8;
				cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::LEFT:
			for (int j = 0; j < bufferSize;) {
				i = b[j++];
				i += b[j++] << 8;
				j += 2;
				cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::RIGHT:
			for (int j = 0; j < bufferSize;) {
				j += 2;
				i = b[j++];
				i += b[j++] << 8;
				cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::IQ:
			for (int j = 0; j < bufferSize;) {
				i = b[j++];
				i += b[j++] << 8;
				q = b[j++];
				q += b[j++] << 8;
				cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::INVIQ:
			for (int j = 0; j < bufferSize;) {
				q = b[j++];
				q += b[j++] << 8;
				i = b[j++];
				i += b[j++] << 8;
				cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;
		}
	}

	free(b);
}

void QGInputAlsa::stop() {
	_running = false;
}
