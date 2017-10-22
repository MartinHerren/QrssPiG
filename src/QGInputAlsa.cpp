#include "QGInputAlsa.h"

#include <iostream>
#include <stdexcept>

QGInputAlsa::QGInputAlsa(const YAML::Node &config): QGInputDevice(config), _device(nullptr) {
	_deviceName = "hw:0,0";

	if (config["device"]) _deviceName = config["device"].as<std::string>();

	int err;
	snd_pcm_hw_params_t *hw_params;

	if ((err = snd_pcm_open(&_device, _deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0))) throw std::runtime_error(std::string("Error opening audio device: ") + snd_strerror(err));

	if ((err = snd_pcm_hw_params_malloc(&hw_params))) throw std::runtime_error(std::string("Error allocating audio device params: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_any(_device, hw_params))) throw std::runtime_error(std::string("Error getting audio device params: ") + snd_strerror(err));

	if ((err = snd_pcm_hw_params_set_access(_device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))) throw std::runtime_error(std::string("Error setting audio device access: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_set_format(_device, hw_params, SND_PCM_FORMAT_S16_LE))) throw std::runtime_error(std::string("Error setting audio device format: ") + snd_strerror(err));
	if ((err = snd_pcm_hw_params_set_channels(_device, hw_params, 2))) throw std::runtime_error(std::string("Error setting audio device channels: ") + snd_strerror(err));
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
	unsigned char b[4096 * 2 * 2]; // 2 channels => 2 short values per sample TODO: support mono and stereo iq, left and right, 8, 16 and 24 bits
	_running = true;

	int err;
	signed short int r;

	while (_running) {
		if ((err = snd_pcm_readi(_device, b, samplesPerCall)) != samplesPerCall) {
			if (err < 0) {
				throw std::runtime_error(std::string("Error reading from audio device: ") + snd_strerror(err));
			} else {
				samplesPerCall = err; // push only residual read
				_running = false;
			}
		}

		for (int j = 0; j < samplesPerCall * 4;) {
			r = b[j++];
			r += b[j++] << 8;
			j += 2;
			cb(std::complex<float>(r / 32768., 0.));
		}
	}
}

void QGInputAlsa::stop() {
	_running = false;
}
