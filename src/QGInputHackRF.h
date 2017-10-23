#pragma once

#include <libhackrf/hackrf.h>

#include "QGInputDevice.h"

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
