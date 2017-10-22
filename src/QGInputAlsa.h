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
	std::string _deviceName;
	snd_pcm_t *_device;

	volatile bool _running;
};
