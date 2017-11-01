#pragma once

#include "QGInputDevice.h"

#include <rtl-sdr.h>

class QGInputRtlSdr: public QGInputDevice {
public:
	QGInputRtlSdr(const YAML::Node &config, std::function<void(std::complex<float>)>cb);
	~QGInputRtlSdr();

	void deviceList();

	void run();
	void stop();

	void process(unsigned char *buf, uint32_t len);

	static void async(unsigned char *buf, uint32_t len, void *ctx);

private:
	int _deviceIndex;

	rtlsdr_dev_t *_device;
};
