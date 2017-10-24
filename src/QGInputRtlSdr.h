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

private:
	int _deviceIndex;

	rtlsdr_dev_t *_device;
};
