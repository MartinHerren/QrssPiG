#include "QGInputAlsa.h"

#include <iostream>
#include <stdexcept>

QGInputAlsa::QGInputAlsa(const YAML::Node &config, std::function<void(std::complex<float>)>cb): QGInputDevice(config, cb), _device(nullptr), _async(nullptr) {
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

	// TODO depending on format
	_bytesPerSample = 2;

	_samplesPerCall = 4096;
	_bufferSize = _samplesPerCall * _bytesPerSample * _numChannels;
	_buffer.reset(new unsigned char[_bufferSize]);

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
	stop();
	if (_device) snd_pcm_close(_device);
}

void QGInputAlsa::run() {
	_running.lock(); // lock to running state.

	int err;

	if ((err = snd_async_add_pcm_handler(&_async, _device, this->async, this))) throw std::runtime_error(std::string("Error setting async handler: ") + snd_strerror(err));
	if ((err = snd_pcm_start(_device))) throw std::runtime_error(std::string("Error starting async capture: ") + snd_strerror(err));

	_running.lock(); // block until lock as been freed by stop()
	_running.unlock(); // Cleanup

	return;
}

void QGInputAlsa::stop() {
	if (!_running.try_lock()) {
		snd_pcm_drain(_device); // or snd_pcm_drop for immediate stop
		snd_async_del_handler(_async);
		_async = nullptr;
	}
	_running.unlock();// Enable run() to return, or release the lock we just gained if it was not running
}

void QGInputAlsa::process() {
	int err;

	snd_pcm_sframes_t avail = snd_pcm_avail_update(_device);

	while (avail >= _samplesPerCall) {
		// successfull reads < _samplesPerCall should not happen due to while condition
		if ((err = snd_pcm_readi(_device, _buffer.get(), _samplesPerCall)) != _samplesPerCall) {
			throw std::runtime_error(std::string("Error reading from audio device: ") + snd_strerror(err));
		}

		signed short int i = 0, q = 0;

		// Only S16_LE supported so far
		switch (_channel) {
		case Channel::MONO:
			for (int j = 0; j < _bufferSize;) {
				i = _buffer[j++];
				i += _buffer[j++] << 8;
				_cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::LEFT:
			for (int j = 0; j < _bufferSize;) {
				i = _buffer[j++];
				i += _buffer[j++] << 8;
				j += 2;
				_cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::RIGHT:
			for (int j = 0; j < _bufferSize;) {
				j += 2;
				i = _buffer[j++];
				i += _buffer[j++] << 8;
				_cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::IQ:
			for (int j = 0; j < _bufferSize;) {
				i = _buffer[j++];
				i += _buffer[j++] << 8;
				q = _buffer[j++];
				q += _buffer[j++] << 8;
				_cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;

		case Channel::INVIQ:
			for (int j = 0; j < _bufferSize;) {
				q = _buffer[j++];
				q += _buffer[j++] << 8;
				i = _buffer[j++];
				i += _buffer[j++] << 8;
				_cb(std::complex<float>(i / 32768., q / 32768.));
			}
			break;
		}

		avail = snd_pcm_avail_update(_device);
	}
}

void QGInputAlsa::async(snd_async_handler_t *async) {
	try {
		static_cast<QGInputAlsa *>(snd_async_handler_get_callback_private(async))->process();
	} catch (const std::exception &e) {
		// TODO prepare device again if error is from pcm device read and ignore if from fft ?
	}
}
