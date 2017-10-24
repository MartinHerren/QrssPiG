#pragma once

#include "QGInputDevice.h"

#include <libhackrf/hackrf.h>

class QGInputHackRF: public QGInputDevice {
public:
	QGInputHackRF(const YAML::Node &config);
	~QGInputHackRF();

	void deviceList();

	void run(std::function<void(std::complex<float>)>cb);
	void stop();

private:
	hackrf_device *_device;
};
