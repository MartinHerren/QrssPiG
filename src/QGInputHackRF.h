#pragma once

#include <libhackrf/hackrf.h>

#include "QGInputDevice.h"

class QGInputHackRF: public QGInputDevice {
public:
	QGInputHackRF(const YAML::Node &config);
	~QGInputHackRF();

	void open();

private:
	hackrf_device *_device;
};
