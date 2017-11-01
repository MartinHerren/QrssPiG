#pragma once

#include "QGInputDevice.h"

class QGInputStdIn: public QGInputDevice {
public:
	QGInputStdIn(const YAML::Node &config, std::function<void(std::complex<float>)>cb);
	~QGInputStdIn();

	void run();
	void stop();

private:
	enum class Format { U8IQ, S8IQ, U16IQ, S16IQ, S16MONO, S16LEFT, S16RIGHT };

	Format _format;

	volatile bool _running;
};
