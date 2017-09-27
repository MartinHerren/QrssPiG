#pragma once

#include <libhackrf/hackrf.h>

#include "QGInput.h"

class QGInputHackRF: public QGInput {
public:
	QGInputHackRF();
	~QGInputHackRF();

	void open();

private:
	hackrf_device *_hackrf;
};
