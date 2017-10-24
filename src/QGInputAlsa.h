#pragma once

#include "QGInputDevice.h"

#include <mutex>

#include <alsa/asoundlib.h>

class QGInputAlsa: public QGInputDevice {
public:
	QGInputAlsa(const YAML::Node &config);
	~QGInputAlsa();

	void run(std::function<void(std::complex<float>)>cb);
	void stop();

	void process();

	static void async(snd_async_handler_t *async);

private:
	enum class Channel { MONO, LEFT, RIGHT, IQ, INVIQ };

	std::string _deviceName;
	Channel _channel;

	int _numChannels;
	int _bytesPerSample;
	int _samplesPerCall;
	int _bufferSize;
	unsigned char *_buffer;
	snd_pcm_t *_device;
	snd_async_handler_t *_async;
	std::function<void(std::complex<float>)>_cb;

	std::mutex _running;
};
