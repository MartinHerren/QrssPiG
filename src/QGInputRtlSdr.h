#pragma once

#include "QGInputDevice.h"

#include <thread>

#include <rtl-sdr.h>

class QGInputRtlSdr: public QGInputDevice {
public:
	QGInputRtlSdr(const YAML::Node &config);
	~QGInputRtlSdr();

	void deviceList();

	static void async(unsigned char *buf, uint32_t len, void *ctx);

private:
	void _startDevice();
	void _stopDevice();

	void _process(unsigned char *buf, uint32_t len);

	int _deviceIndex;

	std::thread _t;
	rtlsdr_dev_t *_device;
};
