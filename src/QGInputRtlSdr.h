#pragma once

#include <rtl-sdr.h>

#include "QGInputDevice.h"

class QGInputRtlSdr: public QGInputDevice {
public:
	QGInputRtlSdr(const YAML::Node &config);
	~QGInputRtlSdr();

	void open();

private:
};
