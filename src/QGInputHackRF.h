#pragma once

#include "QGInputDevice.h"

#include <mutex>

#include <libhackrf/hackrf.h>

class QGInputHackRF: public QGInputDevice {
public:
	QGInputHackRF(const YAML::Node &config, std::function<void(std::complex<float>)>cb);
	~QGInputHackRF();

	void deviceList();

	void run();
	void stop();

	void process(uint8_t *buf, int len);

	static int async(hackrf_transfer* transfer);

private:
	hackrf_device *_device;

	std::mutex _running;
};
