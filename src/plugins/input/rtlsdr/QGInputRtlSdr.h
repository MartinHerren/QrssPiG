#pragma once

#include "QGInputDevice.h"

#include <thread>

#include <rtl-sdr.h>

class QGInputRtlSdr: public QGInputDevice {
public:
	static std::vector<std::string> listDevices();

	QGInputRtlSdr(const YAML::Node &config);
	~QGInputRtlSdr();

private:
	void _startDevice();
	void _stopDevice();

	void _process(unsigned char *buf, uint32_t len);

	int _validGain(float gain);

public:
	static void async(unsigned char *buf, uint32_t len, void *ctx);

private:
	enum class DirectSampling { OFF = 0, I_BRANCH = 1, Q_BRANCH = 2 };

	std::thread _t;
	rtlsdr_dev_t *_device;
};
