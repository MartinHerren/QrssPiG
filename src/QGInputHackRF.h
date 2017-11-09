#pragma once

#include "QGInputDevice.h"

#include <mutex>

#include <libhackrf/hackrf.h>

class QGInputHackRF: public QGInputDevice {
public:
	QGInputHackRF(const YAML::Node &config);
	~QGInputHackRF();

	void deviceList();

	static int async(hackrf_transfer* transfer);

private:
	void _startDevice();
	void _stopDevice();

	void _process(uint8_t *buf, int len);

	hackrf_device *_device;
};
