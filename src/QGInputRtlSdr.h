#pragma once

#include "QGInputDevice.h"

#include <rtl-sdr.h>

class QGInputRtlSdr: public QGInputDevice {
public:
	QGInputRtlSdr(const YAML::Node &config);
	~QGInputRtlSdr();

	void deviceList();

	void run(std::function<void(std::complex<float>)>cb);
	void stop();

	void process(unsigned char *buf, uint32_t len);

	static void async(unsigned char *buf, uint32_t len, void *ctx);

private:
	int _deviceIndex;

	rtlsdr_dev_t *_device;
	std::function<void(std::complex<float>)>_cb;
};
