#pragma once

#include "QGInputDevice.h"

#include <libhackrf/hackrf.h>

class QGInputHackRF: public QGInputDevice {
public:
	static std::vector<std::string> listDevices();

	QGInputHackRF(const YAML::Node &config);
	~QGInputHackRF();

private:
	void _startDevice();
	void _stopDevice();

	void _process(uint8_t *buf, int len);

public:
	static int async(hackrf_transfer* transfer);

	hackrf_device *_device;
};
