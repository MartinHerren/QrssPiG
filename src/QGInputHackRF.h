#pragma once

#include "QGInputDevice.h"

#include <vector>
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

	std::mutex _bufferMutex;
	size_t _bufferCapacity;
	size_t _bufferSize;
	volatile size_t _bufferHead;
	volatile size_t _bufferTail;
	std::vector<std::complex<float>> _buffer;
};
