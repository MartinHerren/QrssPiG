#pragma once

#include "QGInputDevice.h"

#include <alsa/asoundlib.h>

class QGInputAlsa: public QGInputDevice {
public:
	QGInputAlsa(const YAML::Node &config);
	~QGInputAlsa();

	void run(std::function<void(std::complex<float>)>cb);
	void stop();

private:
	enum class Channel { MONO, LEFT, RIGHT, IQ, INVIQ };

	std::string _deviceName;
	Channel _channel;

	snd_pcm_t *_device;
	int _numChannels;
	int _bytesPerSample;

	volatile bool _running;
};
